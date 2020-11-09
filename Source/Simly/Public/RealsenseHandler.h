#pragma once

#include "Engine.h"
#include "Core.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"

#include <map>
#include <string>
#include <exception>
#include <vector>

#include "RealSenseNative.h"
#include "RealSensePlugin.h"
#include "RealSenseContext.h"
#include "RealSenseDevice.h"
#include "RealSenseSensor.h"
#include "RealSenseOption.h"
#include "RealSenseTypes.h"

#include "PointcloudInterface.h"

#include "RealSenseHandler.generated.h"

#pragma once
DECLARE_LOG_CATEGORY_EXTERN(LogPointCloud, Log, All);

UCLASS(ClassGroup = "Simly", BlueprintType)
class SIMLY_API ARealSenseHandler : public AActor, public IPointCloudInterface
{
	GENERATED_UCLASS_BODY()
	friend class FRealSenseHandlerWorker;
	friend class FRealSenseHandlerCustomization;

public:

	virtual ~ARealSenseHandler();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Simly")
		void OnPointCloudAvailable(AActor* Caller);

	UPROPERTY(Category = "RealSense", BlueprintReadOnly)
		class URealSenseContext* Context;

	UPROPERTY(Category = "RealSense", BlueprintReadOnly)
		class URealSenseDevice* ActiveDevice;

	UFUNCTION(Category = "RealSense", BlueprintCallable)
		virtual bool Start();                                                                                     

	UFUNCTION(Category = "RealSense", BlueprintCallable)
		virtual void Stop();

	UFUNCTION(Category = "RealSense", BlueprintCallable)
		void PollFrame(FTransform transform, bool Append);

	// Point Cloud
	UPROPERTY(Category = "Simly", BlueprintReadOnly)
		ULidarPointCloud* PointCloud;

	UPROPERTY(Category = "Simly", BlueprintReadOnly)
		TArray<FLidarPointCloudPoint> Points;

	UFUNCTION(Category = "Simly", BlueprintCallable)
		void SavePointCloud();

	// Device
	UPROPERTY(Category = "Device", BlueprintReadWrite, EditAnywhere)
		ERealSensePipelineMode PipelineMode = ERealSensePipelineMode::CaptureOnly;

	UPROPERTY(Category = "Device", BlueprintReadWrite, EditAnywhere)
		FString CaptureFile;

	// Depth
	UPROPERTY(Category = "Stream", BlueprintReadWrite, EditAnywhere)
		FRealSenseStreamMode DepthConfig;

	UPROPERTY(Category = "Depth", BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0", ClampMax = "1000", UIMin = "0", UIMax = "1000"))
		float DepthMin = 0;

	UPROPERTY(Category = "Depth", BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.1", ClampMax = "1000", UIMin = "0.1", UIMax = "1000"))
		float DepthMax = 10;

	UPROPERTY(Category = "Depth", BlueprintReadWrite, EditAnywhere)
		float DepthScale = 0.01f;

	UPROPERTY(Category = "Depth", BlueprintReadWrite, EditAnywhere)
		float ScaleX = 0.002227171492f;

	UPROPERTY(Category = "Depth", BlueprintReadWrite, EditAnywhere)
		float ScaleY = 0.002325581395f;

	// Color
	UPROPERTY(Category = "Stream", BlueprintReadWrite, EditAnywhere)
		FRealSenseStreamMode ColorConfig;

	// Infrared
	UPROPERTY(Category = "Stream", BlueprintReadWrite, EditAnywhere)
		FRealSenseStreamMode InfraredConfig;

protected:

	virtual void Tick(float DeltaSeconds) override; 
	virtual void BeginPlay() override;

private:

	void ProcessFrameset(class rs2::frameset* Frameset, FTransform Transform, bool Append);
	void EnsureProfileSupported(class URealSenseDevice* Device, ERealSenseStreamType StreamType, ERealSenseFormatType Format, FRealSenseStreamMode Mode);

	FCriticalSection StateMx;

	TUniquePtr<class rs2::pipeline> RsPipeline;
	TUniquePtr<class rs2::device> RsDevice;
	TUniquePtr<class rs2::align> RsAlign;

	volatile int StartedFlag = false;
	volatile int FramesetId = 0;
	bool FirstFrame = false;

	struct rgba
	{
		uint8 r;
		uint8 g;
		uint8 b;
		uint8 a;
	};
};