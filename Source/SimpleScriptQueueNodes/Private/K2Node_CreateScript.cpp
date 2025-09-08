// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#include "K2Node_CreateScript.h"
#include "SimpleScript.h"
#include "UObject/UnrealType.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "BlueprintNodeSpawner.h"
#include "EditorCategoryUtils.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "ScriptQueueComponent.h"
#include "Kismet/GameplayStatics.h"
#include "K2Node_CallFunction.h"
#include "KismetCompilerMisc.h"
#include "KismetCompiler.h"
#include "K2Node_VariableGet.h"
#include "Editor/BlueprintGraph/Classes/EdGraphSchema_K2_Actions.h"

//=================================================================================================
// 
//=================================================================================================
struct FK2Node_SimpleScriptHelper
{
	static FName ClassPinName;
	static FName OuterPinName;
	static FName AddScriptToQueue;
	static FName CreateScript;
	static FName RepeatCount;
};

FName FK2Node_SimpleScriptHelper::ClassPinName(TEXT("Class"));
FName FK2Node_SimpleScriptHelper::OuterPinName(TEXT("WorldContext"));
FName FK2Node_SimpleScriptHelper::CreateScript(TEXT("Node_CreateScript"));
FName FK2Node_SimpleScriptHelper::AddScriptToQueue(TEXT("Node_AddScriptToQueue"));
FName FK2Node_SimpleScriptHelper::RepeatCount(TEXT("RepeatCount"));

//
#define LOCTEXT_NAMESPACE "K2Node_CreateScript"

//=================================================================================================
// 
//=================================================================================================
UK2Node_CreateScript::UK2Node_CreateScript(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NodeTooltip = FText::FromString(TEXT("Add a new queued event"));
}

//=================================================================================================
// 
//=================================================================================================
UClass* UK2Node_CreateScript::GetClassPinBaseClass() const
{
	return USimpleScript::StaticClass();
}

//=================================================================================================
// 
//=================================================================================================
void UK2Node_CreateScript::AllocateDefaultPins()
{
	UEdGraphSchema_K2 const* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Add execution pins
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	// If required add the world context pin
	if (GetBlueprint()->ParentClass->HasMetaDataHierarchical(FBlueprintMetadata::MD_ShowWorldContextPin))
	{
		CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), FK2Node_SimpleScriptHelper::OuterPinName);
	}

	// Add blueprint pin
	UEdGraphPin* ClassPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Class, GetClassPinBaseClass(), FK2Node_SimpleScriptHelper::ClassPinName);

	// Transform pin
	UEdGraphPin* RepeatCountPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Int, FK2Node_SimpleScriptHelper::RepeatCount);
	K2Schema->ConstructBasicPinTooltip(*RepeatCountPin, LOCTEXT("RepeatCountDescription", "How many times can this CLASS of script triggered. Zero means infinite repeats."), RepeatCountPin->PinToolTip);

	// Result pin
	UEdGraphPin* ResultPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Object, GetClassPinBaseClass(), UEdGraphSchema_K2::PN_ReturnValue);
	K2Schema->ConstructBasicPinTooltip(*ResultPin, LOCTEXT("ResultPinDescription", "The created script"), ResultPin->PinToolTip);

	Super::AllocateDefaultPins();
}

//=================================================================================================
// 
//=================================================================================================
UEdGraphPin* UK2Node_CreateScript::GetOuterPin() const
{
	UEdGraphPin* Pin = FindPin(FK2Node_SimpleScriptHelper::OuterPinName);
	ensure(nullptr == Pin || Pin->Direction == EGPD_Input);
	return Pin;
}

//=================================================================================================
// 
//=================================================================================================
UEdGraphPin* UK2Node_CreateScript::GetResultPin() const
{
	UEdGraphPin* Pin = FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue);
	check(Pin->Direction == EGPD_Output);
	return Pin;
}

//=================================================================================================
// 
//=================================================================================================
void UK2Node_CreateScript::SetPinToolTip(UEdGraphPin& MutatablePin, const FText& PinDescription) const
{
	MutatablePin.PinToolTip = UEdGraphSchema_K2::TypeToText(MutatablePin.PinType).ToString();

	UEdGraphSchema_K2 const* const K2Schema = Cast<const UEdGraphSchema_K2>(GetSchema());
	if (K2Schema != nullptr)
	{
		MutatablePin.PinToolTip += TEXT(" ");
		MutatablePin.PinToolTip += K2Schema->GetPinDisplayName(&MutatablePin).ToString();
	}

	MutatablePin.PinToolTip += FString(TEXT("\n")) + PinDescription.ToString();
}

//=================================================================================================
// 
//=================================================================================================
void UK2Node_CreateScript::CreatePinsForClass(UClass* InClass, TArray<UEdGraphPin*>* OutClassPins)
{
	check(InClass);

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	const UObject* const ClassDefaultObject = InClass->GetDefaultObject(false);

	for (TFieldIterator<FProperty> PropertyIt(InClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
	{
		FProperty* Property = *PropertyIt;
		//UClass* PropertyClass = CastChecked<UClass>(Property->GetClass());
		const bool bIsDelegate = Property->IsA(FMulticastDelegateProperty::StaticClass());
		const bool bIsExposedToSpawn = UEdGraphSchema_K2::IsPropertyExposedOnSpawn(Property);
		const bool bIsSettableExternally = !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);

		if (bIsExposedToSpawn &&
			!Property->HasAnyPropertyFlags(CPF_Parm) &&
			bIsSettableExternally &&
			Property->HasAllPropertyFlags(CPF_BlueprintVisible) &&
			!bIsDelegate &&
			(nullptr == FindPin(Property->GetFName())) &&
			FBlueprintEditorUtils::PropertyStillExists(Property))
		{
			if (UEdGraphPin* Pin = CreatePin(EGPD_Input, NAME_None, Property->GetFName()))
			{
				K2Schema->ConvertPropertyToPinType(Property, /*out*/ Pin->PinType);
				if (OutClassPins)
				{
					OutClassPins->Add(Pin);
				}

				if (ClassDefaultObject && K2Schema->PinDefaultValueIsEditable(*Pin))
				{
					FString DefaultValueAsString;
					const bool bDefaultValueSet = FBlueprintEditorUtils::PropertyValueToString(Property, reinterpret_cast<const uint8*>(ClassDefaultObject), DefaultValueAsString);
					check(bDefaultValueSet);
					K2Schema->SetPinAutogeneratedDefaultValue(Pin, DefaultValueAsString);
				}

				// Copy tooltip from the property.
				K2Schema->ConstructBasicPinTooltip(*Pin, Property->GetToolTipText(), Pin->PinToolTip);
			}
		}
	}

	// Change class of output pin
	UEdGraphPin* ResultPin = GetResultPin();
	ResultPin->PinType.PinSubCategoryObject = InClass;
}

//=================================================================================================
// 
//=================================================================================================
UClass* UK2Node_CreateScript::GetClassToSpawn(const TArray<UEdGraphPin*>* InPinsToSearch /*=NULL*/) const
{
	UClass* UseSpawnClass = nullptr;
	const TArray<UEdGraphPin*>* PinsToSearch = InPinsToSearch ? InPinsToSearch : &Pins;

	UEdGraphPin* ClassPin = GetClassPin(PinsToSearch);
	if (ClassPin && ClassPin->DefaultObject && ClassPin->LinkedTo.Num() == 0)
	{
		UseSpawnClass = CastChecked<UClass>(ClassPin->DefaultObject);
	}
	else if (ClassPin && ClassPin->LinkedTo.Num())
	{
		UEdGraphPin* ClassSource = ClassPin->LinkedTo[0];
		UseSpawnClass = ClassSource ? Cast<UClass>(ClassSource->PinType.PinSubCategoryObject.Get()) : nullptr;
	}

	return UseSpawnClass;
}

//=================================================================================================
// 
//=================================================================================================
void UK2Node_CreateScript::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	AllocateDefaultPins();
	if (UClass* UseSpawnClass = GetClassToSpawn(&OldPins))
	{
		CreatePinsForClass(UseSpawnClass);
	}
	RestoreSplitPins(OldPins);
}

//=================================================================================================
// 
//=================================================================================================
void UK2Node_CreateScript::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();

	if (UClass* UseSpawnClass = GetClassToSpawn())
	{
		CreatePinsForClass(UseSpawnClass);
	}
}

//=================================================================================================
// 
//=================================================================================================
UEdGraphPin* UK2Node_CreateScript::GetRepeatCountPin() const
{
	UEdGraphPin* Pin = FindPinChecked(FK2Node_SimpleScriptHelper::RepeatCount);
	check(Pin->Direction == EGPD_Input);
	return Pin;
}

//=================================================================================================
// 
//=================================================================================================
bool UK2Node_CreateScript::IsSpawnVarPin(UEdGraphPin* Pin) const
{
	return	Pin->PinName != UEdGraphSchema_K2::PN_Execute &&
			Pin->PinName != UEdGraphSchema_K2::PN_Then &&
			Pin->PinName != UEdGraphSchema_K2::PN_Self &&
			Pin->PinName != UEdGraphSchema_K2::PN_ReturnValue &&
			Pin->PinName != FK2Node_SimpleScriptHelper::ClassPinName &&
			Pin->PinName != FK2Node_SimpleScriptHelper::OuterPinName &&
			Pin->PinName != FK2Node_SimpleScriptHelper::RepeatCount;
}

//=================================================================================================
// 
//=================================================================================================
void UK2Node_CreateScript::OnClassPinChanged()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Because the archetype has changed, we break the output link as the output pin type will change
	UEdGraphPin* ResultPin = GetResultPin();
	ResultPin->BreakAllPinLinks();

	// Remove all pins related to archetype variables
	TArray<UEdGraphPin*> OldPins = Pins;
	TArray<UEdGraphPin*> OldClassPins;

	for (UEdGraphPin* OldPin : OldPins)
	{
		if (IsSpawnVarPin(OldPin))
		{
			Pins.Remove(OldPin);
			OldClassPins.Add(OldPin);
		}
	}

	CachedNodeTitle.MarkDirty();

	TArray<UEdGraphPin*> NewClassPins;
	if (UClass* UseSpawnClass = GetClassToSpawn())
	{
		CreatePinsForClass(UseSpawnClass, &NewClassPins);
	}

	RestoreSplitPins(OldPins);

	// Rewire the old pins to the new pins so connections are maintained if possible
	RewireOldPinsToNewPins(OldClassPins, Pins, nullptr);

	// Refresh the UI for the graph so the pin changes show up
	GetGraph()->NotifyGraphChanged();

	// Mark dirty
	FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}

//=================================================================================================
// 
//=================================================================================================
void UK2Node_CreateScript::PinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::PinConnectionListChanged(Pin);

	if (Pin && (Pin->PinName == FK2Node_SimpleScriptHelper::ClassPinName))
	{
		OnClassPinChanged();
	}
}

//=================================================================================================
// 
//=================================================================================================
void UK2Node_CreateScript::GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const
{
	if (UEdGraphPin* ClassPin = GetClassPin())
	{
		SetPinToolTip(*ClassPin, FText::FromString(TEXT("The object class you want to construct")));
	}

	return Super::GetPinHoverText(Pin, HoverTextOut);
}

//=================================================================================================
// 
//=================================================================================================
void UK2Node_CreateScript::PinDefaultValueChanged(UEdGraphPin* ChangedPin)
{
	if (ChangedPin && (ChangedPin->PinName == FK2Node_SimpleScriptHelper::ClassPinName))
	{
		OnClassPinChanged();
	}
}

//=================================================================================================
// 
//=================================================================================================
FText UK2Node_CreateScript::GetTooltipText() const
{
	return NodeTooltip;
}

//=================================================================================================
// 
//=================================================================================================
UEdGraphPin* UK2Node_CreateScript::GetThenPin()const
{
	UEdGraphPin* Pin = FindPinChecked(UEdGraphSchema_K2::PN_Then);
	check(Pin->Direction == EGPD_Output);
	return Pin;
}

//=================================================================================================
// 
//=================================================================================================
UEdGraphPin* UK2Node_CreateScript::GetClassPin(const TArray<UEdGraphPin*>* InPinsToSearch /*= NULL*/) const
{
	const TArray<UEdGraphPin*>* PinsToSearch = InPinsToSearch ? InPinsToSearch : &Pins;

	UEdGraphPin* Pin = nullptr;
	for (UEdGraphPin* TestPin : *PinsToSearch)
	{
		if (TestPin && TestPin->PinName == FK2Node_SimpleScriptHelper::ClassPinName)
		{
			Pin = TestPin;
			break;
		}
	}
	check(Pin == nullptr || Pin->Direction == EGPD_Input);
	return Pin;
}

//=================================================================================================
// 
//=================================================================================================
FText UK2Node_CreateScript::GetBaseNodeTitle() const
{
	return FText::FromString(TEXT("Create Simple Script"));
}

//=================================================================================================
// 
//=================================================================================================
FText UK2Node_CreateScript::GetNodeTitleFormat() const
{
	return FText::FromString(TEXT("Add Script To Queue: {ClassName}"));
}

//=================================================================================================
// 
//=================================================================================================
FText UK2Node_CreateScript::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
	{
		return GetBaseNodeTitle();
	}
	else if (UClass* ClassToSpawn = GetClassToSpawn())
	{
		if (CachedNodeTitle.IsOutOfDate(this))
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("ClassName"), ClassToSpawn->GetDisplayNameText());

			// FText::Format() is slow, so we cache this to save on performance
			CachedNodeTitle.SetCachedText(FText::Format(GetNodeTitleFormat(), Args), this);
		}
		return CachedNodeTitle;
	}
	return FText::FromString(TEXT("Add Script To Queue: NONE"));
}

//=================================================================================================
// 
//=================================================================================================
bool UK2Node_CreateScript::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph);
	return Super::IsCompatibleWithGraph(TargetGraph) && (!Blueprint || FBlueprintEditorUtils::FindUserConstructionScript(Blueprint) != TargetGraph);
}

//=================================================================================================
// 
//=================================================================================================
void UK2Node_CreateScript::GetNodeAttributes(TArray<TKeyValuePair<FString, FString>>& OutNodeAttributes) const
{
	UClass* ClassToSpawn = GetClassToSpawn();
	const FString ClassToSpawnStr = ClassToSpawn ? ClassToSpawn->GetName() : TEXT("InvalidClass");
	OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("Type"), TEXT("AddScriptToQueue")));
	OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("Class"), GetClass()->GetName()));
	OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("Name"), GetName()));
	OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("ObjectClass"), ClassToSpawnStr));
}

//=================================================================================================
// 
//=================================================================================================
void UK2Node_CreateScript::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	UK2Node_CreateScript* TemplateNode = NewObject<UK2Node_CreateScript>(GetTransientPackage(), GetClass());

	const FString Category = TEXT("Heroine");
	const FText   MenuDesc = LOCTEXT("CreateScript", "Create Queue Script");
	const FString Tooltip = TEXT("Add a script into queue");

	TSharedPtr<FEdGraphSchemaAction_K2NewNode> NodeAction = TSharedPtr<FEdGraphSchemaAction_K2NewNode>(new FEdGraphSchemaAction_K2NewNode(FText::FromString(Category), MenuDesc, FText::FromString(Tooltip), 0));

	//FK2ActionMenuBuilder::AddNewNodeAction(ContextMenuBuilder, Category, MenuDesc, Tooltip);
	NodeAction->NodeTemplate = TemplateNode;

	ContextMenuBuilder.AddAction(NodeAction, Category);
}

//=================================================================================================
// 
//=================================================================================================
void UK2Node_CreateScript::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	// actions get registered under specific object-keys; the idea is that 
	// actions might have to be updated (or deleted) if their object-key is  
	// mutated (or removed)... here we use the node's class (so if the node 
	// type disappears, then the action should go with it)
	UClass* ActionKey = GetClass();
	// to keep from needlessly instantiating a UBlueprintNodeSpawner, first   
	// check to make sure that the registrar is looking for actions of this type
	// (could be regenerating actions for a specific asset, and therefore the 
	// registrar would only accept actions corresponding to that asset)
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

//=================================================================================================
// 
//=================================================================================================
FText UK2Node_CreateScript::GetMenuCategory() const
{
	return FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::Gameplay);
}

//=================================================================================================
// 
//=================================================================================================
bool UK2Node_CreateScript::HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const
{
	const UBlueprint* SourceBlueprint = GetBlueprint();

	UClass* SourceClass1 = GetClassToSpawn();
	const bool bResult1 = (SourceClass1 && (SourceClass1->ClassGeneratedBy != SourceBlueprint));
	if (bResult1 && OptionalOutput)
	{
		OptionalOutput->AddUnique(SourceClass1);
	}

	const bool bResult2 = false;

	const bool bSuperResult = Super::HasExternalDependencies(OptionalOutput);
	return bSuperResult || bResult1 || bResult2;
}

//=================================================================================================
// 
//=================================================================================================
struct FK2Node_CreateScript_Utils
{
	static bool CanSpawnObjectOfClass(TSubclassOf<UObject> ObjectClass, bool bAllowAbstract)
	{
		// Initially include types that meet the basic requirements.
		// Note: CLASS_Deprecated is an inherited class flag, so any subclass of an explicitly-deprecated class also cannot be spawned.
		bool bCanSpawnObject = (nullptr != *ObjectClass)
			&& (bAllowAbstract || !ObjectClass->HasAnyClassFlags(CLASS_Abstract))
			&& !ObjectClass->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists);

		if (bCanSpawnObject)
		{
			static const FName BlueprintTypeName(TEXT("BlueprintType"));
			static const FName NotBlueprintTypeName(TEXT("NotBlueprintType"));
			static const FName DontUseGenericSpawnObjectName(TEXT("DontUseGenericSpawnObject"));

			// Exclude all types in the initial set by default.
			bCanSpawnObject = false;
			const UClass* CurrentClass = ObjectClass;

			// Climb up the class hierarchy and look for "BlueprintType." If "NotBlueprintType" is seen first, or if the class is not allowed, then stop searching.
			while (!bCanSpawnObject && CurrentClass != nullptr && !CurrentClass->GetBoolMetaData(NotBlueprintTypeName))
			{
				// Include any type that either includes or inherits 'BlueprintType'
				bCanSpawnObject = CurrentClass->GetBoolMetaData(BlueprintTypeName);

				// Stop searching if we encounter 'BlueprintType' with 'DontUseGenericSpawnObject'
				if (bCanSpawnObject && CurrentClass->GetBoolMetaData(DontUseGenericSpawnObjectName))
				{
					bCanSpawnObject = false;
					break;
				}

				CurrentClass = CurrentClass->GetSuperClass();
			}
		}

		return bCanSpawnObject;
	}
};

//=================================================================================================
// 
//=================================================================================================
void UK2Node_CreateScript::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	//const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

	static const FName ObjectParamName(TEXT("Object"));
	static const FName ValueParamName(TEXT("Value"));
	static const FName PropertyNameParamName(TEXT("PropertyName"));

	UK2Node_CreateScript * SpawnNode = this;
	UEdGraphPin* SpawnNodeExec = SpawnNode->GetExecPin();
	UEdGraphPin* SpawnWorldContextPin = SpawnNode->GetOuterPin();

	UEdGraphPin* SpawnClassPin = SpawnNode->GetClassPin();
	UEdGraphPin* SpawnNodeThen = SpawnNode->GetThenPin();
	UEdGraphPin* SpawnNodeResult = GetResultPin();
	UEdGraphPin* SpawnNodeRepeatCount = GetRepeatCountPin();

	//////////////////////////////////////////////////////////////////////////
	// create 'begin spawn' call node
	UK2Node_CallFunction* CallBeginSpawnNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(SpawnNode, SourceGraph);
	CallBeginSpawnNode->FunctionReference.SetExternalMember(FK2Node_SimpleScriptHelper::CreateScript, UScriptQueueComponent::StaticClass());
	CallBeginSpawnNode->AllocateDefaultPins();

	if (!CallBeginSpawnNode)
	{
		CompilerContext.MessageLog.Error(TEXT("Create event function not found for @@."), SpawnNode);
		// we break exec links so this is the only error we get, don't want the SpawnActor node being considered and giving 'unexpected node' type warnings
		SpawnNode->BreakAllNodeLinks();
		return;
	}

	// Cache the class to spawn. Note, this is the compile time class that the pin was set to or the variable type it was connected to. Runtime it could be a child.
	UClass* ClassToSpawn = GetClassToSpawn();

	UClass* SpawnClass = (SpawnClassPin != NULL) ? Cast<UClass>(SpawnClassPin->DefaultObject) : NULL;
	if (!SpawnClassPin || ((0 == SpawnClassPin->LinkedTo.Num()) && (NULL == SpawnClass)))
	{
		CompilerContext.MessageLog.Error(TEXT("Add to queue node @@ must have a @@ specified."), SpawnNode, SpawnClassPin);
		// we break exec links so this is the only error we get, don't want the SpawnActor node being considered and giving 'unexpected node' type warnings
		SpawnNode->BreakAllNodeLinks();
		return;
	}

	/*
	if (!SpawnWorldContextPin || (0 == SpawnWorldContextPin->LinkedTo.Num()))
	{ 
		CompilerContext.MessageLog.Error(TEXT("Add to queue node @@ must have a @@ specified."), SpawnNode, SpawnWorldContextPin);
		// we break exec links so this is the only error we get, don't want the SpawnActor node being considered and giving 'unexpected node' type warnings
		SpawnNode->BreakAllNodeLinks();
		return;
	}
	*/



	UEdGraphPin* CallBeginExec = CallBeginSpawnNode->GetExecPin();
	UEdGraphPin* CallBeginWorldContextPin = CallBeginSpawnNode->FindPin(FK2Node_SimpleScriptHelper::OuterPinName); //Schema->FindSelfPin(*CallBeginSpawnNode, EGPD_Input);  //CallBeginSpawnNode->FindPinChecked(FK2Node_AddQueuedEventHelper::OuterPinName);
	UEdGraphPin* CallBeginRepeatCount = CallBeginSpawnNode->FindPinChecked(FK2Node_SimpleScriptHelper::RepeatCount);

	/*
	if (!CallBeginWorldContextPin)
	{
		CompilerContext.MessageLog.Error(TEXT("Failed to find ScriptQueueComponent in @@."), SpawnNode);
		// we break exec links so this is the only error we get, don't want the SpawnActor node being considered and giving 'unexpected node' type warnings
		SpawnNode->BreakAllNodeLinks();
		return;
	}
	*/

	// Copy the world context connection from the spawn node to 'begin spawn' if necessary
	if (SpawnWorldContextPin)
	{
		CompilerContext.MovePinLinksToIntermediate(*SpawnWorldContextPin, *CallBeginWorldContextPin);
	}

	// Copy the 'transform' connection from the spawn node to 'begin spawn'
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeRepeatCount, *CallBeginRepeatCount);

	UEdGraphPin* CallBeginActorClassPin = CallBeginSpawnNode->FindPinChecked(FK2Node_SimpleScriptHelper::ClassPinName);
	if (!CallBeginActorClassPin)
	{
		CompilerContext.MessageLog.Error(TEXT("Failed to find Classs in @@."), SpawnNode);
		// we break exec links so this is the only error we get, don't want the SpawnActor node being considered and giving 'unexpected node' type warnings
		SpawnNode->BreakAllNodeLinks();
		return;
	}

	UEdGraphPin* CallBeginResult = CallBeginSpawnNode->GetReturnValuePin();

	// Move 'exec' connection from spawn node to 'begin spawn'
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeExec, *CallBeginExec);

	if (SpawnClassPin->LinkedTo.Num() > 0)
	{
		// Copy the 'blueprint' connection from the spawn node to 'begin spawn'
		CompilerContext.MovePinLinksToIntermediate(*SpawnClassPin, *CallBeginActorClassPin);
	}
	else
	{
		// Copy blueprint literal onto begin spawn call 
		CallBeginActorClassPin->DefaultObject = SpawnClass;
	}

	// create 'finish spawn' call node
	UK2Node_CallFunction* CallAddScriptToQueue = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(SpawnNode, SourceGraph);
	CallAddScriptToQueue->FunctionReference.SetExternalMember(FK2Node_SimpleScriptHelper::AddScriptToQueue, UScriptQueueComponent::StaticClass());
	CallAddScriptToQueue->AllocateDefaultPins();

	static const FName ScriptName(TEXT("Script"));

	UEdGraphPin* CallFinishExec = CallAddScriptToQueue->GetExecPin();
	UEdGraphPin* CallFinishThen = CallAddScriptToQueue->GetThenPin();
	UEdGraphPin* CallFinishActor = CallAddScriptToQueue->FindPinChecked(ScriptName);
	UEdGraphPin* CallFinishResult = CallAddScriptToQueue->GetReturnValuePin();

	// Move 'then' connection from spawn node to 'finish spawn'
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeThen, *CallFinishThen);

	// Connect output actor from 'begin' to 'finish'
	CallBeginResult->MakeLinkTo(CallFinishActor);

	// Move result connection from spawn node to 'finish spawn'
	CallFinishResult->PinType = SpawnNodeResult->PinType; // Copy type so it uses the right actor subclass
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeResult, *CallFinishResult);

	//////////////////////////////////////////////////////////////////////////
	// create 'set var' nodes

	// Get 'result' pin from 'begin spawn', this is the actual actor we want to set properties on
	UEdGraphPin* LastThen = FKismetCompilerUtilities::GenerateAssignmentNodes(CompilerContext, SourceGraph, CallBeginSpawnNode, SpawnNode, CallBeginResult, ClassToSpawn);

	// Make exec connection between 'then' on last node and 'finish'
	LastThen->MakeLinkTo(CallFinishExec);

	// Break any links to the expanded node
	SpawnNode->BreakAllNodeLinks();
}

//=================================================================================================
// 
//=================================================================================================
void UK2Node_CreateScript::EarlyValidation(class FCompilerResultsLog& MessageLog) const
{
	Super::EarlyValidation(MessageLog);
	UEdGraphPin* ClassPin = GetClassPin(&Pins);
	const bool bAllowAbstract = ClassPin && ClassPin->LinkedTo.Num();
	UClass* ClassToSpawn = GetClassToSpawn();
	if (!FK2Node_CreateScript_Utils::CanSpawnObjectOfClass(ClassToSpawn, bAllowAbstract))
	{
		MessageLog.Error(*FString::Printf(TEXT("Cannot add queued event of type '%s' in @@"), *GetPathNameSafe(ClassToSpawn)), this);
	}
}


#undef LOCTEXT_NAMESPACE
