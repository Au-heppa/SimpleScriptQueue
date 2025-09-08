// Copyright Epic Games, Inc. All Rights Reserved.

#include "SimpleScriptQueue.h"

#define LOCTEXT_NAMESPACE "FSimpleScriptQueuetModule"

void FSimpleScriptQueueModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FSimpleScriptQueueModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSimpleScriptQueueModule, SimpleScriptQueue)