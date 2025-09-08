// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EdGraph/EdGraphNodeUtils.h"
#include "K2Node.h"
#include "Engine/MemberReference.h"
#include "K2Node_ConstructObjectFromClass.h"
#include "K2Node_CreateScript.generated.h"

class FBlueprintActionDatabaseRegistrar;
class UEdGraph;

//=================================================================================================
// 
//=================================================================================================
UCLASS()
class SIMPLESCRIPTQUEUENODES_API UK2Node_CreateScript : public UK2Node
{
	GENERATED_UCLASS_BODY()

	//~ Begin UEdGraphNode Interface.
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void EarlyValidation(class FCompilerResultsLog& MessageLog) const override;
	//~ End UEdGraphNode Interface.

	//~ Begin UEdGraphNode Interface.
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual FText GetTooltipText() const override;
	virtual bool HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const override;
	virtual bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin);
	virtual void GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const override;
	virtual void PostPlacedNewNode() override;
	//~ End UEdGraphNode Interface.

	//~ Begin UK2Node Interface
	virtual bool IsNodeSafeToIgnore() const override { return true; }
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
	virtual void GetNodeAttributes(TArray<TKeyValuePair<FString, FString>>& OutNodeAttributes) const override;
	virtual void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FText GetMenuCategory() const override;
	//~ End UK2Node Interface

	/** Create new pins to show properties on archetype */
	void CreatePinsForClass(UClass* InClass, TArray<UEdGraphPin*>* OutClassPins = nullptr);

	/** See if this is a spawn variable pin, or a 'default' pin */
	virtual bool IsSpawnVarPin(UEdGraphPin* Pin) const;

	/** Get the then output pin */
	UEdGraphPin* GetThenPin() const;
	/** Get the blueprint input pin */
	UEdGraphPin* GetClassPin(const TArray<UEdGraphPin*>* InPinsToSearch = NULL) const;

	/** Get the class that we are going to spawn, if it's defined as default value */
	UClass* GetClassToSpawn(const TArray<UEdGraphPin*>* InPinsToSearch = NULL) const;

protected:
	/** Gets the default node title when no class is selected */
	virtual FText GetBaseNodeTitle() const;
	/** Gets the node title when a class has been selected. */
	virtual FText GetNodeTitleFormat() const;
	/** Gets base class to use for the 'class' pin.  UObject by default. */
	virtual UClass* GetClassPinBaseClass() const;
	
	/***/
	virtual UEdGraphPin* GetOuterPin() const;

	/** Get the result output pin */
	virtual	UEdGraphPin* GetResultPin() const;

	/** Get the spawn transform input pin */
	UEdGraphPin* GetRepeatCountPin() const;

	/**
	* Takes the specified "MutatablePin" and sets its 'PinToolTip' field (according
	* to the specified description)
	*
	* @param   MutatablePin	The pin you want to set tool-tip text on
	* @param   PinDescription	A string describing the pin's purpose
	*/
	void SetPinToolTip(UEdGraphPin& MutatablePin, const FText& PinDescription) const;

	/** Refresh pins when class was changed */
	void OnClassPinChanged();

	/** Tooltip text for this node. */
	FText NodeTooltip;

	/** Constructing FText strings can be costly, so we cache the node's title */
	FNodeTextCache CachedNodeTitle;

	UPROPERTY()
	FMemberReference				MemberVariableToCallOn;
};