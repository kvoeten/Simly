/*
*   Copyright 2022 Kaz Voeten
*
*	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
*	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "MediaReader.h"

AMediaReader::AMediaReader(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

AMediaReader::~AMediaReader()
{
	StartedFlag = false;
	if (Thread.Get())
	{
		{
			Thread->WaitForCompletion();
		}
		Thread.Reset();
	}
	Worker.Reset();
	if (this->FileHandle)
	{
		delete FileHandle;
	}
}

void AMediaReader::BeginPlay()
{
	this->PointCloud = NewObject<ULidarPointCloud>();
	Super::BeginPlay();
}

void AMediaReader::Initialize(FString FileName)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.FileExists(*FileName))
	{
		UE_LOG(LogTemp, Warning, TEXT("FILE NOT FOUND!"));
		return;
	}

	this->FileHandle = PlatformFile.OpenRead(*FileName);
	if (this->FileHandle)
	{
		// Get File Header
		uint8 head[12];
		this->FileHandle->Read(head, 12);
		const FILEHEADER* header = (const FILEHEADER*)head;

		// Set frame variables
		this->Width = header->width;
		this->Height = header->height;
		this->DEPTH_BUFFER = new uint8[sizeof(uint16) * Width * Height];
		this->COLOR_BUFFER = new uint8[sizeof(uint32) * Width * Height];

		// Initialize point-cloud
		for (int i = 0; i < Width * Height; ++i)
			this->Points.Add(FLidarPointCloudPoint(0, 0, -1000 * i, 0, 0, 0));

		this->PointCloud->SetData(Points);

		// Initialize worker thread
		StartedFlag = true;
		Worker.Reset(new FMediaReaderWorker(this));
		StartTime = (uint64)FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64());
		FString ThreadName(FString::Printf(TEXT("FRealSenseInspectorWorker_%s"), *FGuid::NewGuid().ToString()));
		Thread.Reset(FRunnableThread::Create(Worker.Get(), *ThreadName, 0, TPri_Normal));
		if (!Thread.Get())UE_LOG(LogTemp, Fatal, TEXT("Unable to create thread"));

		UE_LOG(LogTemp, Log, TEXT("[Point Cloud Video] Got header, resolution: %d : %d"), header->width, header->height);
	}
}

void AMediaReader::ThreadProc()
{
	if (this->FileHandle)
	{
		while (StartedFlag)
		{
			uint64 PassedTime = (uint64) FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64()) - StartTime;

			// Check if the file has another frame header
			if (FileHandle->Size() - FileHandle->Tell() < sizeof(uint64))
			{
				UE_LOG(LogTemp, Warning, TEXT("[Point Cloud Video] End of file."));
				StartedFlag = false;
				delete FileHandle;
				return;
			}
			
			// Get Timestamp
			uint8 head[sizeof(uint64)];
			FileHandle->Read(head, sizeof(uint64));
			
			uint64 timestamp = ((uint64)head[7] << 56)
				| ((uint64)head[6] << 48)
				| ((uint64)head[5] << 40)
				| ((uint64)head[4] << 32)
				| ((uint64)head[3] << 24)
				| ((uint64)head[2] << 16)
				| ((uint64)head[1] << 8)
				| ((uint64)head[0]);				

			// Check if ready for next frame
			if (PassedTime >= timestamp)
			{
				// Get color & depth data
				UE_LOG(LogTemp, Warning, TEXT("[Point Cloud Video] Updating PCL: pt%d, ts:%d"), PassedTime, timestamp);
				FileHandle->Read(DEPTH_BUFFER, this->Width * this->Height * sizeof(uint16));
				FileHandle->Read(COLOR_BUFFER, this->Width * this->Height * sizeof(RGBA));
				UpdatePointCloud();
			}
			else
			{
				// Move reader back to be able to read timestamp again.
				FileHandle->Seek(FileHandle->Tell() - sizeof(uint64));
			}
		}
	}
}

void AMediaReader::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AMediaReader::UpdatePointCloud()
{
	const int centerx = 0.5 * Width;
	const int centery = 0.5 * Height;

	const uint16_t* depth_data = (const uint16_t*)DEPTH_BUFFER;
	const RGBA* color_data = (const RGBA*)COLOR_BUFFER;
	
	for (int i = 0; i < Width * Height; ++i)
	{
		const uint16_t depth = depth_data[i];
		const RGBA color = color_data[i];

		float z = depth * DepthScale;
		float x = ((i % Width) - centerx - 0.5f) * z * ScaleX;
		float y = ((i / Width) - centery - 0.5f) * z * ScaleY;

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

	PointCloud->SetData(Points);
}