/*
*   Copyright 2022 Kaz Voeten
*
*	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
*	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "Core.h"
#include "Engine.h"
#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "MovieSceneMediaSection.h"
#include "LidarPointCloudShared.h"
#include "LidarPointCloud.h"

#include <exception>
#include <vector>
#include <map>

#include "MediaReader.generated.h"

UCLASS(ClassGroup = "Simly", BlueprintType)
class SIMLY_API AMediaReader : public AActor
{
	GENERATED_UCLASS_BODY()
	friend class FMediaReaderWorker;

public:

	virtual ~AMediaReader();

	// Point Cloud
	UPROPERTY(Category = "Simly", BlueprintReadOnly)
		ULidarPointCloud* PointCloud;

	UPROPERTY(Category = "Simly", BlueprintReadOnly)
		TArray<FLidarPointCloudPoint> Points;

	UFUNCTION(Category = "Simly", BlueprintCallable)
		void Initialize(FString FileName);

	UFUNCTION(Category = "Simly", BlueprintCallable)
		void UpdatePointCloud();

	// Depth
	UPROPERTY(Category = "Depth", BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0", ClampMax = "1000", UIMin = "0", UIMax = "1000"))
		float DepthMin = 0;

	UPROPERTY(Category = "Depth", BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.1", ClampMax = "1000", UIMin = "0.1", UIMax = "1000"))
		float DepthMax = 10;

	UPROPERTY(Category = "Depth", BlueprintReadWrite, EditAnywhere)
		float DepthScale = 0.001f;

	UPROPERTY(Category = "Depth", BlueprintReadWrite, EditAnywhere)
		float ScaleX = 0.002227171492f;

	UPROPERTY(Category = "Depth", BlueprintReadWrite, EditAnywhere)
		float ScaleY = 0.002325581395f;

protected:

	virtual void Tick(float DeltaSeconds) override; 
	virtual void BeginPlay() override;

private:
	void ThreadProc();
	TUniquePtr<class FMediaReaderWorker> Worker;
	TUniquePtr<class FRunnableThread> Thread;
	volatile int StartedFlag = false;

	IFileHandle* FileHandle;
	uint8* COLOR_BUFFER;
	uint8* DEPTH_BUFFER;
	int Width, Height;
	uint64 StartTime, TargetTime;
	TArray<FLidarPointCloudPoint*> aPoints;

	struct RGBA
	{
		uint8 r;
		uint8 g;
		uint8 b;
		uint8 a;
	};

	struct FILEHEADER
	{
		uint32 width;
		uint32 height;
		uint32 encryption;
	};
};

class FMediaReaderWorker : public FRunnable
{
public:
	FMediaReaderWorker(AMediaReader* Context) { this->Context = Context; }
	virtual ~FMediaReaderWorker() {}
	virtual bool Init() { return true; }
	virtual uint32 Run() { Context->ThreadProc(); return 0; }
	virtual void Stop() { Context->StartedFlag = false; }

private:
	AMediaReader* Context = nullptr;
};