#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LidarPointCloudShared.h"
#include "LidarPointCloud.h"
#include "PointCloudInterface.generated.h"

UINTERFACE(MinimalAPI)
class UPointCloudInterface : public UInterface
{
	GENERATED_BODY()
};

class SIMLY_API IPointCloudInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Simly")
		void OnPointCloudAvailable(AActor* Caller);
};