// Fill out your copyright notice in the Description page of Project Settings.

#include "SimpleScriptQueueNodes.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FSimpleScriptQueueNodes_Module, SimpleScriptQueueNodes, "SimpleScriptQueueNodes" );

//=========================================================================================================================
// 
//=========================================================================================================================
void FSimpleScriptQueueNodes_Module::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

}

//=========================================================================================================================
// 
//=========================================================================================================================
void FSimpleScriptQueueNodes_Module::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.


}
