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
class Simly_API AMediaReader : public AActor
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