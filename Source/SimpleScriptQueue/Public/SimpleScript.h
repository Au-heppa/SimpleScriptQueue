// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "Engine/Classes/Engine/LatentActionManager.h"
#include "SimpleScript.generated.h"

//============================================================================================================
//
//============================================================================================================
UCLASS(BlueprintType, Abstract, Blueprintable)
class SIMPLESCRIPTQUEUE_API USimpleScript : public UObject
{
public:
	GENERATED_BODY()

	//Constructor
	USimpleScript();

	//
	virtual bool Initialize(class UScriptQueueComponent* InComponent);

	//
	FORCEINLINE class UScriptQueueComponent *GetComponent() const { return QueueComponent.Get(); }

	class UWorld *GetWorld() const override;

	//
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsActive() const { return bActive; }

	//Called immediately when the object is created
	UFUNCTION(BlueprintNativeEvent)
	void OnAddedToQueue();
	virtual void OnAddedToQueue_Implementation() { }

	//Called earliest the next tick
	UFUNCTION(BlueprintNativeEvent)
	void OnActivate();
	virtual void OnActivate_Implementation() { }

	//Clean up any hard references to actors or components.
	//Invalidate any timers and cancel any gameplay tasks
	UFUNCTION(BlueprintNativeEvent)
	void OnDeactivate(bool Success);
	virtual void OnDeactivate_Implementation(bool Success) { }

	//
	virtual void Activate();

	//
	UFUNCTION(BlueprintCallable)
	virtual void Deactivate(bool Success = true);

	//============================================================================================================
	//
	//============================================================================================================
public:

	//
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSimpleScriptEvent, class USimpleScript *, Script);

	//
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSimpleScriptSuccessEvent, class USimpleScript*, Script, bool, Success);

	//
	UPROPERTY(BlueprintAssignable)
	FSimpleScriptEvent OnStarted;

	//
	UPROPERTY(BlueprintAssignable)
	FSimpleScriptEvent OnCancelled;

	//
	UPROPERTY(BlueprintAssignable)
	FSimpleScriptSuccessEvent OnFinished;

	//
	virtual void ClearAll()
	{
		OnStarted.Clear();
		OnCancelled.Clear();
		OnFinished.Clear();
	}

private:

	mutable TWeakObjectPtr<UWorld> CachedWorld;

public:

	//
	FORCEINLINE bool GetIsInstant() const { return bInstant; }
	FORCEINLINE bool GetUsePool() const { return bUseScriptPool; }

	//============================================================================================================
	//
	//============================================================================================================
protected:

	//
	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (AllowPrivateAccess = true), VisibleAnywhere, Category = "Runtime", AdvancedDisplay)
	TWeakObjectPtr<class UScriptQueueComponent> QueueComponent = NULL;

	//
	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (AllowPrivateAccess = true), VisibleAnywhere, Category = "Runtime", AdvancedDisplay)
	bool bActive;

	//============================================================================================================
	//
	//============================================================================================================
protected:

	//Set to true for scripts that have the potential to fire a lot of times per map.
	//For one time story type of scripts you can set this to false.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bUseScriptPool = true;

	//If the script should go into the "Queue" or "InstantScripts" array.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ExposeOnSpawn=true), Category="Settings")
	bool bInstant;
};




