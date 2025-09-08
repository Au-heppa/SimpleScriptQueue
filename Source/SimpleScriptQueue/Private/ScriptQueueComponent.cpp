// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#include "ScriptQueueComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "SimpleScript.h"


//============================================================================================================
//
//============================================================================================================
UScriptQueueComponent::UScriptQueueComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bAutoActivate = false;
}

//============================================================================================================
//
//============================================================================================================
class UScriptQueueComponent* UScriptQueueComponent::GetScriptQueueComponent(class UObject* WorldContext)
{
	class APlayerController *pController = UGameplayStatics::GetPlayerController(WorldContext, 0);
	if (!IsValid(pController))
		return NULL;

	return pController->FindComponentByClass<UScriptQueueComponent>();
}

/*
//============================================================================================================
//
//============================================================================================================
bool UScriptQueueComponent::CancelScriptsOfClass(class UObject* WorldContext, TSubclassOf<USimpleScript> Class, bool IncludeActive)
{
	class UScriptQueueComponent *pComponent = GetScriptQueueComponent(WorldContext);
	if (!IsValid(pComponent))
		return false;

	return pComponent->CancelScriptsOfClass_Internal(Class, IncludeActive);
}

//============================================================================================================
//
//============================================================================================================
bool UScriptQueueComponent::CancelScriptsOfClass_Internal(TSubclassOf<USimpleScript> Class, bool IncludeActive)
{
	if (IncludeActive)
	{
		TArray<class USimpleScript*> InstantCopy = InstantScripts;
		for (int32 i=0; i< InstantCopy.Num(); i++)
		{
			InstantCopy.GetData()[i]->Deactivate();
		}
	}

	for (int32 i=Queue.Num()-1; i>=0; i--)
	{
		if (i == 0)
		{
			if (!IncludeActive)
				continue;

			Queue.GetData()[i]->Deactivate();
			continue;
		}

		Queue.
	}
}
*/

//============================================================================================================
//
//============================================================================================================
void UScriptQueueComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Queue.Num() > 0)
	{
		if (IsValid(Queue.GetData()[0]))
		{
			Queue.GetData()[0]->Activate();
		}
		else
		{
			Queue.RemoveAt(0);
		}
	}

	for (int32 i=InstantScripts.Num()-1; i>=0; i--)
	{
		if (!IsValid(InstantScripts.GetData()[i]))
		{
			InstantScripts.RemoveAt(i);
		}
	}

	TArray<class USimpleScript*> InstantCopy = InstantScripts;
	for (int32 i=0; i<InstantCopy.Num(); i++)
	{
		if (IsValid(InstantCopy.GetData()[i]))
		{
			InstantCopy.GetData()[i]->Activate();
		}
	}
}

//============================================================================================================
//
//============================================================================================================
void UScriptQueueComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

//============================================================================================================
//
//============================================================================================================
bool UScriptQueueComponent::HasScriptInQueue(TSubclassOf<USimpleScript> Class) const
{
	for (int32 i=0; i<Queue.Num(); i++)
	{
		if (IsValid(Queue.GetData()[i]) && Queue.GetData()[i]->GetClass() == Class)
			return true;
	}

	for (int32 i = 0; i < InstantScripts.Num(); i++)
	{
		if (IsValid(InstantScripts.GetData()[i]) && InstantScripts.GetData()[i]->GetClass() == Class)
			return true;
	}

	return false;
}

//============================================================================================================
//
//============================================================================================================
void UScriptQueueComponent::FinishScript(class USimpleScript *Script, bool Success)
{
	if (!IsValid(Script))
	{
		return;
	}

	if (PoolSize != 0 && Script->GetUsePool())
	{
		if (PoolSize > 0 && ScriptPool.Num() >= PoolSize)
		{
			ScriptPool.RemoveAt(0);
		}

		ScriptPool.Add(Script);
	}

	//Increase repeat counts
	int32 iCurrent = Counts.Contains(Script->GetClass()) ? Counts[Script->GetClass()] : 0;
	Counts.Emplace(Script->GetClass(), iCurrent+1);

	if (Queue.Num() > 0 && Queue.GetData()[0] == Script)
	{
		Queue.RemoveAt(0);
	}
	else if (InstantScripts.Contains(Script))
	{
		InstantScripts.Remove(Script);
	}

	if (InstantScripts.Num() == 0 && Queue.Num() == 0)
	{
		OnQueueFinished.Broadcast();

		PrimaryComponentTick.SetTickFunctionEnable(false);
	}
}

//============================================================================================================
//
//============================================================================================================
class USimpleScript* UScriptQueueComponent::Node_CreateScript(class UObject* WorldContext, TSubclassOf<USimpleScript> Class, int32 RepeatCount)
{
	if (!IsValid(Class))
	{
		return NULL;
	}

	UE_LOG(LogTemp, Display, TEXT("%s input count %d"), *Class->GetName(), RepeatCount);

	class UScriptQueueComponent *pComponent = GetScriptQueueComponent(WorldContext);
	if (!IsValid(pComponent))
	{
		return NULL;
	}

	if (RepeatCount > 0 && pComponent->GetRepeatCount(Class))
	{
		return NULL;
	}

	//Use one from pool if we have it
	int32 i = INDEX_NONE;
	for (int32 j=0; j<pComponent->ScriptPool.Num(); j++)
	{
		if (pComponent->ScriptPool.GetData()[j]->GetClass() == Class)
		{
			i = pComponent->CreatedScripts.Add(pComponent->ScriptPool.GetData()[j]);
			pComponent->ScriptPool.RemoveAt(j);
			break;
		}
	}

	if (i == INDEX_NONE)
	{
		i = pComponent->CreatedScripts.Add(NewObject<USimpleScript>(WorldContext, Class));
	}

	class USimpleScript *pScript = pComponent->CreatedScripts.GetData()[i];
	pScript->Initialize(pComponent);
	return pScript;
}

//============================================================================================================
//
//============================================================================================================
void UScriptQueueComponent::Activate(bool bReset)
{
	Super::Activate();

	PrimaryComponentTick.SetTickFunctionEnable(InstantScripts.Num() > 0 || Queue.Num() > 0);
}

//============================================================================================================
//
//============================================================================================================
void UScriptQueueComponent::Deactivate()
{
	Super::Deactivate();

	PrimaryComponentTick.SetTickFunctionEnable(false);
}

//============================================================================================================
//
//============================================================================================================
void UScriptQueueComponent::AddScriptToQueue(class USimpleScript* Script)
{
	if (!IsValid(Script))
		return;

	if (Script->GetIsInstant())
	{
		InstantScripts.Add(Script);

		OnScriptAdded.Broadcast(Script);

		Script->OnAddedToQueue();

		PrimaryComponentTick.SetTickFunctionEnable(IsActive());
	}
	else
	{
		Queue.Add(Script);

		OnScriptAdded.Broadcast(Script);

		Script->OnAddedToQueue();
	
		PrimaryComponentTick.SetTickFunctionEnable(IsActive());
	}

	CreatedScripts.Reset();
}

//============================================================================================================
//
//============================================================================================================
class USimpleScript *UScriptQueueComponent::Node_AddScriptToQueue(class USimpleScript* Script)
{
	if (IsValid(Script) && IsValid(Script->GetComponent()))
	{
		Script->GetComponent()->AddScriptToQueue(Script);
		return Script;
	}

	return NULL;
}
