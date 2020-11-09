// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SimlyBPLibrary.h"
#include "Simly.h"

USimlyBPLibrary::USimlyBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

TArray<FLidarPointCloudPoint> USimlyBPLibrary::RealsenseToLidar(float Points)
{
	TArray<FLidarPointCloudPoint> PointCloud;
	FLidarPointCloudPoint BasePoint = FLidarPointCloudPoint(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f); //XYZ, RGBA
	PointCloud.Add(BasePoint);
	return PointCloud;
}

