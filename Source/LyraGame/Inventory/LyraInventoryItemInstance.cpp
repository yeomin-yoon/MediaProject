// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraInventoryItemInstance.h"

#include "Inventory/LyraInventoryItemDefinition.h"
#include "Net/UnrealNetwork.h"

#if UE_WITH_IRIS
#include "Iris/ReplicationSystem/ReplicationFragmentUtil.h"
#endif // UE_WITH_IRIS

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraInventoryItemInstance)

class FLifetimeProperty;

ULyraInventoryItemInstance::ULyraInventoryItemInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ULyraInventoryItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, StatTags);
	DOREPLIFETIME(ThisClass, ItemDef);
}

#if UE_WITH_IRIS
void ULyraInventoryItemInstance::RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags)
{
	using namespace UE::Net;

	// Build descriptors and allocate PropertyReplicationFragments for this object
	FReplicationFragmentUtil::CreateAndRegisterFragmentsForObject(this, Context, RegistrationFlags);
}
#endif // UE_WITH_IRIS

void ULyraInventoryItemInstance::AddStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.AddStack(Tag, StackCount);
}

void ULyraInventoryItemInstance::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.RemoveStack(Tag, StackCount);
}

int32 ULyraInventoryItemInstance::GetStatTagStackCount(FGameplayTag Tag) const
{
	return StatTags.GetStackCount(Tag);
}

bool ULyraInventoryItemInstance::HasStatTag(FGameplayTag Tag) const
{
	return StatTags.ContainsTag(Tag);
}

void ULyraInventoryItemInstance::SetItemDef(TSubclassOf<ULyraInventoryItemDefinition> InDef)
{
	ItemDef = InDef;
}

const ULyraInventoryItemFragment* ULyraInventoryItemInstance::FindFragmentByClass(TSubclassOf<ULyraInventoryItemFragment> FragmentClass) const
{
	if ((ItemDef != nullptr) && (FragmentClass != nullptr))
	{
		return GetDefault<ULyraInventoryItemDefinition>(ItemDef)->FindFragmentByClass(FragmentClass);
	}

	return nullptr;
}

FText ULyraInventoryItemInstance::GetDisplayNameByOption() const
{
	switch (OptionType)
	{
	case EItemOptionType::Attack:
		return FText::FromString(TEXT("Shard of Attack"));

	case EItemOptionType::Health:
		return FText::FromString(TEXT("Shard of Vitality"));

	case EItemOptionType::Stamina:
		return FText::FromString(TEXT("Shard of Endurance"));

	default:
		return FText::FromString(TEXT("Unknown Shard"));
	}
}

UTexture2D* ULyraInventoryItemInstance::GetIconTexture() const
{
	FString Prefix;
	int32 MaxIndex = 0;

	switch (OptionType)
	{
	case EItemOptionType::Attack:
		Prefix = TEXT("RedFragment_");
		MaxIndex = 14;
		break;

	case EItemOptionType::Health:
		Prefix = TEXT("YellowFragment_");
		MaxIndex = 13;
		break;

	case EItemOptionType::Stamina:
		Prefix = TEXT("BlueFragment_");
		MaxIndex = 11;
		break;

	default:
		return nullptr;
	}

	FRandomStream Stream(RandomSeed);

	int32 IconIndex =
		Stream.RandRange(0, MaxIndex);

	FString AssetPath = FString::Printf(
		TEXT("/Game/Loot_Drop_VFX/LootUIIMG/%s%d.%s%d"),
		*Prefix,
		IconIndex,
		*Prefix,
		IconIndex
	);

	return LoadObject<UTexture2D>(
		nullptr,
		*AssetPath
	);
}