#include "RealSenseHandler.h"

DEFINE_LOG_CATEGORY(LogPointCloud);

#define MAX_BUFFER_U16 0xFFFF

inline float GetDepthScale(rs2::device dev) {
	for (auto& sensor : dev.query_sensors()) {
		if (auto depth = sensor.as<rs2::depth_sensor>()) {
			return depth.get_depth_scale();
		}
	}
	throw rs2::error("Depth not supported");
}

ARealSenseHandler::ARealSenseHandler(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

ARealSenseHandler::~ARealSenseHandler()
{
	Stop();
}

void ARealSenseHandler::BeginPlay()
{
	Super::BeginPlay();
	this->PointCloud = NewObject<ULidarPointCloud>();
	Start();
}

bool ARealSenseHandler::Start()
{
	try
	{
		FScopeLock Lock(&StateMx);

		// Check Pipeline
		if (RsPipeline.Get() || StartedFlag) throw std::runtime_error("Already started");

		// Get Device context
		Context = IRealSensePlugin::Get().GetContext();
		if (!Context) throw std::runtime_error("GetContext failed");
		rs2::context_ref RsContext(Context->GetHandle());
		rs2::config RsConfig;

		// Get device
		const bool IsPlaybackMode = (PipelineMode == ERealSensePipelineMode::PlaybackFile);
		if (IsPlaybackMode) RsConfig.enable_device_from_file(TCHAR_TO_ANSI(*CaptureFile));
		else
		{
			if (Context->Devices.Num() == 0)
			{
				Context->QueryDevices();
				if (Context->Devices.Num() == 0)
				{
					throw std::runtime_error("No devices available");
				}
			}

			ActiveDevice = Context->GetDeviceById(0);
			if (!ActiveDevice)
			{
				throw std::runtime_error("Device not found");
			}

			RsConfig.enable_device(std::string(TCHAR_TO_ANSI(*ActiveDevice->Serial)));
		}

		// Enable Device
		if (!IsPlaybackMode)
		{
			// Enable Depth Cam
			EnsureProfileSupported(ActiveDevice, ERealSenseStreamType::STREAM_DEPTH, ERealSenseFormatType::FORMAT_Z16, DepthConfig);
			RsConfig.enable_stream(RS2_STREAM_DEPTH, DepthConfig.Width, DepthConfig.Height, RS2_FORMAT_Z16, DepthConfig.Rate);
			RsAlign.Reset(new rs2::align(RS2_STREAM_COLOR));

			// Enable Color Cam
			EnsureProfileSupported(ActiveDevice, ERealSenseStreamType::STREAM_COLOR, ERealSenseFormatType::FORMAT_RGBA8, ColorConfig);
			RsConfig.enable_stream(RS2_STREAM_COLOR, ColorConfig.Width, ColorConfig.Height, RS2_FORMAT_RGBA8, ColorConfig.Rate);

			// Enable IR
			EnsureProfileSupported(ActiveDevice, ERealSenseStreamType::STREAM_INFRARED, ERealSenseFormatType::FORMAT_Y8, InfraredConfig);
			RsConfig.enable_stream(RS2_STREAM_INFRARED, InfraredConfig.Width, InfraredConfig.Height, RS2_FORMAT_Y8, InfraredConfig.Rate);
		}

		// Enable Recording
		if (PipelineMode == ERealSensePipelineMode::RecordFile)
		{
			RsConfig.enable_record_to_file(TCHAR_TO_ANSI(*CaptureFile));
		}

		FlushRenderingCommands();

		// Init Pointcloud
		for (int i = 0; i < DepthConfig.Width * DepthConfig.Height; ++i) 
			this->Points.Add(FLidarPointCloudPoint(0, 0, -10 * i, 0, 0, 0));

		// Set bounds with some fake points to prevent corruption
		this->Points[0] = FLidarPointCloudPoint(10000, 0, 0, 0, 0, 0);
		this->Points[1] = FLidarPointCloudPoint(0, 10000, 0, 0, 0, 0);
		this->Points[2] = FLidarPointCloudPoint(0, 0, 10000, 0, 0, 0);
		this->Points[3] = FLidarPointCloudPoint(-10000, 0, 0, 0, 0, 0);
		this->Points[4] = FLidarPointCloudPoint(0, -10000, 0, 0, 0, 0);
		this->Points[5] = FLidarPointCloudPoint(0, 0, -10000, 0, 0, 0);

		// Initialize
		this->PointCloud->SetData(Points);

		// Clear Pipeline
		RsPipeline.Reset(new rs2::pipeline());
		rs2::pipeline_profile RsProfile = RsPipeline->start(RsConfig);
		StartedFlag = true;
	}
	catch (const rs2::error & ex)
	{
		auto what(FString(ANSI_TO_TCHAR(ex.what())));
		auto func(FString(ANSI_TO_TCHAR(ex.get_failed_function().c_str())));
		auto args(FString(ANSI_TO_TCHAR(ex.get_failed_args().c_str())));
		StartedFlag = false;
	}
	catch (const std::exception & ex)
	{
		UE_LOG(LogPointCloud, Error, TEXT("ARealSenseHandler::Start exception: %s"), ANSI_TO_TCHAR(ex.what()));
		StartedFlag = false;
	}

	if (!StartedFlag)
	{
		Stop();
	}

	return StartedFlag ? true : false;
}

void ARealSenseHandler::Stop()
{
	try
	{
		FScopeLock Lock(&StateMx);

		StartedFlag = false;

		if (RsPipeline.Get())
		{
			try {
				RsPipeline->stop();
			}
			catch (...) {}
		}

		RsPipeline.Reset();
		RsAlign.Reset();

		{
			ENQUEUE_RENDER_COMMAND(FlushCommand)(
				[](FRHICommandListImmediate& RHICmdList)
				{
					GRHICommandList.GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::FlushRHIThreadFlushResources);
					RHIFlushResources();
					GRHICommandList.GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::FlushRHIThreadFlushResources);
				});
			FlushRenderingCommands();
		}

		ActiveDevice = nullptr;
	}
	catch (const std::exception & ex)
	{
		UE_LOG(LogPointCloud, Error, TEXT("ARealSenseHandler::Stop exception: %s"), ANSI_TO_TCHAR(ex.what()));
	}
}

void ARealSenseHandler::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ARealSenseHandler::PollFrame(FTransform Transform = FTransform(), bool Append = false)
{
	rs2::frameset Frameset;
	try
	{
		RsPipeline->poll_for_frames(&Frameset);
		ProcessFrameset(&Frameset, Transform, Append);
	}
	catch (const rs2::error & ex)
	{
		UE_LOG(LogPointCloud, Error, TEXT("ARealSenseHandler::PollFrames exception: %s"), ANSI_TO_TCHAR(ex.what()));
	}
}

void ARealSenseHandler::ProcessFrameset(rs2::frameset* Frameset, FTransform Transform, bool Append)
{
	const rs2::video_frame& DepthFrame = (RsAlign.Get()) ? RsAlign->process(*Frameset).get_depth_frame() : Frameset->get_depth_frame();
	const rs2::video_frame& ColorFrame = Frameset->get_color_frame();

	const auto width = DepthFrame.get_width();
	const auto height = DepthFrame.get_height();

	const int centerx = 0.5 * width;
	const int centery = 0.5 * height;

	const uint16_t* depth_data = (const uint16_t*)DepthFrame.get_data();
	const rgba* color_data = (const rgba*)ColorFrame.get_data();
	
	for (int i = 0; i < width * height; ++i)
	{
		const uint16_t depth = depth_data[i];
		const rgba color = color_data[i];

		float z = depth * DepthScale;
		float x = ((i % width) - centerx - 0.5f) * z * ScaleX;
		float y = ((i / width) - centery - 0.5f) * z * ScaleY;

		if (Append)
		{
			FVector Local = FVector(-x, -z, -y);
			FVector World = Transform.TransformPosition(Local);
			this->Points[i].Location = World;
			this->Points[i].Color.R = color.r;
			this->Points[i].Color.G = color.g;
			this->Points[i].Color.B = color.b;
		}
		else
		{
			if (z < this->DepthMin || z > this->DepthMax)
			{
				this->Points[i].Location.Set(0, 0, 0);
				this->Points[i].Color.R = 0;
				this->Points[i].Color.G = 0;
				this->Points[i].Color.B = 0;
			}
			else
			{
				this->Points[i].Location.Set(-x, -z, -y);
				this->Points[i].Color.R = color.r;
				this->Points[i].Color.G = color.g;
				this->Points[i].Color.B = color.b;
			}
		}
	}

	if (!Append) PointCloud->SetData(Points);
	else
	{
		if (this->FirstFrame)
		{
			FBox Bounds = FBox();
			Bounds.ExpandBy(FVector(-10000, -10000, -10000), FVector(10000, 10000, 1000));
			PointCloud->Initialize(Bounds);
		}
		PointCloud->InsertPoints(Points, ELidarPointCloudDuplicateHandling::SelectFirst, false, FVector(0, 0, 0));
		PointCloud->RefreshRendering();
	}
	
	this->FirstFrame = false;
	FramesetId++;
}

void ARealSenseHandler::SavePointCloud()
{
	FString SaveFilePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir())
		+ "/PointCloud_" + FDateTime::Today().ToHttpDate() + ".txt";
	this->PointCloud->Export(SaveFilePath);
}

void ARealSenseHandler::OnPointCloudAvailable_Implementation(AActor* Caller)
{
	if (Caller != this) { return; }

	TArray<AActor*> Interfaces;
	UGameplayStatics::GetAllActorsWithInterface(this, UPointCloudInterface::StaticClass(), Interfaces);

	for (const auto& Actor : Interfaces) {
		IPointCloudInterface* Interface = Cast<IPointCloudInterface>(Actor);
		if (Interface) { Interface->Execute_OnPointCloudAvailable(Actor, Caller); }
		else if (Actor->GetClass()->ImplementsInterface(UPointCloudInterface::StaticClass())) 
		{
			IPointCloudInterface::Execute_OnPointCloudAvailable(Actor, Caller);
		}
	}
}

void ARealSenseHandler::EnsureProfileSupported(URealSenseDevice* Device, ERealSenseStreamType StreamType, ERealSenseFormatType Format, FRealSenseStreamMode Mode)
{
	FRealSenseStreamProfile Profile;
	Profile.StreamType = StreamType;
	Profile.Format = Format;
	Profile.Width = Mode.Width;
	Profile.Height = Mode.Height;
	Profile.Rate = Mode.Rate;

	if (!Device->SupportsProfile(Profile))
	{
		throw std::runtime_error("Profile not supported");
	}
}
