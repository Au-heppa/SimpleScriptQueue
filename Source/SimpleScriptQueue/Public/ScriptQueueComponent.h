// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network


#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "SimpleScript.h"
#include "ScriptQueueComponent.generated.h"

//============================================================================================================
//
//============================================================================================================
UCLASS( ClassGroup=(Heroine), meta=(BlueprintSpawnableComponent) )
class SIMPLESCRIPTQUEUE_API UScriptQueueComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UScriptQueueComponent();

public:

	virtual void Activate(bool bReset = false) override;
	virtual void Deactivate() override;

	//
	UFUNCTION(BlueprintCallable, meta = (WorldContext="WorldContext"))
	static class UScriptQueueComponent* GetScriptQueueComponent(class UObject* WorldContext);

	//
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContext", UnsafeDuringActorConstruction = "true", BlueprintInternalUseOnly = "true"))
	static class USimpleScript *Node_CreateScript(class UObject* WorldContext, TSubclassOf<USimpleScript> Class, UPARAM(meta=(MinClamp="0")) int32 RepeatCount = 0);

	//
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContext", UnsafeDuringActorConstruction = "true", BlueprintInternalUseOnly = "true"))
	static class USimpleScript* Node_AddScriptToQueue(class USimpleScript *Script);

	//
	/*
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContext", UnsafeDuringActorConstruction = "true", BlueprintInternalUseOnly = "true"))
	static bool CancelScriptsOfClass(class UObject* WorldContext, TSubclassOf<USimpleScript> Class, bool IncludeActive = true);
	bool CancelScriptsOfClass_Internal(TSubclassOf<USimpleScript> Class, bool IncludeActive);
	*/

public:

	//
	UFUNCTION(BlueprintPure)
	bool HasQueue() const;

	//
	UFUNCTION(BlueprintPure)
	bool HasScriptInQueue(TSubclassOf<USimpleScript> Class) const;

public:

	//
	void FinishScript(class USimpleScript *Script, bool Success);

	//
	void AddScriptToQueue(class USimpleScript* Script);

public:

	//
	void EndPlay(const EEndPlayReason::Type EndPlayReason);

	//
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	//
	FORCEINLINE int32 GetRepeatCount(TSubclassOf<class USimpleScript> Class) const { return Counts.Contains(Class) ? Counts[Class] : 0; }

private:

	//
	UPROPERTY(Category="Pool", EditAnywhere, meta=(ClampMin="-1"))
	int32 PoolSize = 20;

	//Queue
	UPROPERTY(VisibleAnywhere, Category = "Runtime", BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TArray<class USimpleScript*> ScriptPool;

private:

	//Queue
	UPROPERTY(VisibleAnywhere, Category="Runtime", BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	TArray<class USimpleScript*> Queue;

	//
	UPROPERTY(VisibleAnywhere, Category = "Runtime", BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TArray<class USimpleScript*> InstantScripts;

	//
	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Runtime", BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TMap<TSubclassOf<class USimpleScript>, int32> Counts;

public:

	//
	UPROPERTY(VisibleAnywhere, Category = "Runtime", BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TArray<class USimpleScript*> CreatedScripts;

	//============================================================================================================
	//
	//============================================================================================================
public:

	//
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FScriptQueueEvent);

	//
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FScriptQueueScriptEvent, class USimpleScript*, Script);

	//
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FScriptQueueSuccessEvent, class USimpleScript*, Script, bool, Success);

	//
	UPROPERTY(BlueprintAssignable)
	FScriptQueueScriptEvent OnScriptAdded;

	//
	UPROPERTY(BlueprintAssignable)
	FScriptQueueScriptEvent OnScriptStarted;

	//
	UPROPERTY(BlueprintAssignable)
	FScriptQueueScriptEvent OnScriptCancelled;

	//
	UPROPERTY(BlueprintAssignable)
	FScriptQueueSuccessEvent OnScriptFinished;

	//
	UPROPERTY(BlueprintAssignable)
	FScriptQueueEvent OnQueueFinished;

	//
	virtual void ClearAllEvents(class UObject* Object);
};

//============================================================================================================
//
//============================================================================================================
FORCEINLINE bool UScriptQueueComponent::HasQueue() const
{
	return Queue.Num() > 0 || InstantScripts.Num() > 0;
}

//============================================================================================================
//
//============================================================================================================
FORCEINLINE void UScriptQueueComponent::ClearAllEvents(class UObject* Object)
{
	OnScriptAdded.RemoveAll(Object);
	OnScriptStarted.RemoveAll(Object);
	OnScriptFinished.RemoveAll(Object);
	OnQueueFinished.RemoveAll(Object);
}