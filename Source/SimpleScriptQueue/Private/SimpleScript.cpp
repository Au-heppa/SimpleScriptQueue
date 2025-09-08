// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#include "SimpleScript.h"
#include "ScriptQueueComponent.h"

//============================================================================================================
//
//============================================================================================================
USimpleScript::USimpleScript()
{
	QueueComponent = NULL;
}

//=============================================================================================================================
// 
//=============================================================================================================================
class UWorld *USimpleScript::GetWorld() const
{
	if ( UWorld* LastWorld = CachedWorld.Get() )
	{
		return LastWorld;
	}

	if ( HasAllFlags(RF_ClassDefaultObject) )
	{
		// If we are a CDO, we must return nullptr instead of calling Outer->GetWorld() to fool UObject::ImplementsGetWorld.
		return nullptr;
	}

	// Could be a GameInstance, could be World, could also be a WidgetTree, so we're just going to follow
	// the outer chain to find the world we're in.
	UObject* Outer = GetOuter();

	while ( Outer )
	{
		UWorld* World = Outer->GetWorld();
		if ( World )
		{
			CachedWorld = World;
			return World;
		}

		Outer = Outer->GetOuter();
	}

	return nullptr;
}

//============================================================================================================
//
//============================================================================================================
bool USimpleScript::Initialize(class UScriptQueueComponent *InComponent)
{
	if (IsValid(InComponent))
	{
		QueueComponent = InComponent;
		return true;
	}

	return false;
}

//============================================================================================================
//
//============================================================================================================
void USimpleScript::Activate()
{
	if (!bActive && IsValid(QueueComponent.Get()))
	{
		bActive = true;

		QueueComponent->OnScriptStarted.Broadcast(this);
		OnStarted.Broadcast(this);

		OnActivate();
	}
}

//============================================================================================================
//
//============================================================================================================
void USimpleScript::Deactivate(bool WasSuccess)
{
	if (bActive && IsValid(QueueComponent.Get()))
	{
		bActive = false;
		OnDeactivate(WasSuccess);

		OnFinished.Broadcast(this, WasSuccess);
		ClearAll();
		QueueComponent->OnScriptFinished.Broadcast(this, WasSuccess);

		QueueComponent->FinishScript(this, WasSuccess);
	}
}

