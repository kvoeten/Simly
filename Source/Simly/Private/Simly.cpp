// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Simly.h"
#include "RealSenseHardwareCustomization.h"

#define LOCTEXT_NAMESPACE "FSimlyModule"

void FSimlyModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
#if WITH_EDITOR
	FRealSenseHardwareCustomization::Register();
#endif
}

void FSimlyModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
#if WITH_EDITOR
	FRealSenseHardwareCustomization::Unregister();
#endif
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSimlyModule, Simly)