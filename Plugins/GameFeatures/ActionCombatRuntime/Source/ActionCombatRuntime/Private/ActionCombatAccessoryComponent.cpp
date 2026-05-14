#include "ActionCombatAccessoryComponent.h"

#include "ActionCombatAccessoryData.h"
#include "ActionCombatAppearanceComponent.h"
#include "ActionCombatRuntimeLog.h"

#include "Animation/AnimInstance.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/SoftObjectPath.h"

void FActionCombatEquippedAccessoryList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
    if (OwnerComponent)
    {
        for (int32 Index : RemovedIndices)
        {
            OwnerComponent->HandleReplicatedAccessoryRemoved(Entries[Index].SlotTag);
        }
    }
}

void FActionCombatEquippedAccessoryList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
    if (OwnerComponent)
    {
        OwnerComponent->HandleReplicatedAccessoriesChanged();
    }
}

void FActionCombatEquippedAccessoryList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
    if (OwnerComponent)
    {
        OwnerComponent->HandleReplicatedAccessoriesChanged();
    }
}

bool FActionCombatEquippedAccessoryList::SetEntry(FGameplayTag SlotTag, const TSoftObjectPtr<UActionCombatAccessoryData>& AccessoryData)
{
    for (FActionCombatEquippedAccessoryEntry& Entry : Entries)
    {
        if (Entry.SlotTag == SlotTag)
        {
            if (Entry.AccessoryData == AccessoryData)
            {
                return false;
            }

            Entry.AccessoryData = AccessoryData;
            MarkItemDirty(Entry);
            return true;
        }
    }

    FActionCombatEquippedAccessoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
    NewEntry.SlotTag = SlotTag;
    NewEntry.AccessoryData = AccessoryData;
    MarkItemDirty(NewEntry);
    return true;
}

bool FActionCombatEquippedAccessoryList::RemoveEntry(FGameplayTag SlotTag)
{
    for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
    {
        if (EntryIt->SlotTag == SlotTag)
        {
            EntryIt.RemoveCurrent();
            MarkArrayDirty();
            return true;
        }
    }

    return false;
}

bool FActionCombatEquippedAccessoryList::ClearEntries()
{
    if (Entries.IsEmpty())
    {
        return false;
    }

    Entries.Reset();
    MarkArrayDirty();
    return true;
}

UActionCombatAccessoryComponent::UActionCombatAccessoryComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UActionCombatAccessoryComponent::OnRegister()
{
    Super::OnRegister();

    EquippedAccessories.SetOwnerComponent(this);
}

void UActionCombatAccessoryComponent::BeginPlay()
{
    Super::BeginPlay();

    RefreshAccessoryVisuals();
}

void UActionCombatAccessoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    DestroyAllSpawnedAccessoryVisuals();

    Super::EndPlay(EndPlayReason);
}

void UActionCombatAccessoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, EquippedAccessories);
    DOREPLIFETIME(ThisClass, AccessoryVisualSyncRevision);
    DOREPLIFETIME(ThisClass, AuthoritativeAccessoryEntryCount);
}

bool UActionCombatAccessoryComponent::RequestEquipAccessory(UActionCombatAccessoryData* AccessoryData)
{
    if (!AccessoryData)
    {
        return false;
    }

    const FSoftObjectPath AccessoryDataPath(AccessoryData);
    if (!AccessoryDataPath.IsValid())
    {
        return false;
    }

    if (!HasAuthorityForAccessories())
    {
        ServerRequestEquipAccessory(AccessoryDataPath);
        return true;
    }

    return HandleEquipRequest(AccessoryDataPath);
}

bool UActionCombatAccessoryComponent::RequestUnequipSlot(FGameplayTag SlotTag)
{
    if (!SlotTag.IsValid())
    {
        return false;
    }

    if (!HasAuthorityForAccessories())
    {
        ServerRequestUnequipSlot(SlotTag);
        return true;
    }

    return HandleUnequipRequest(SlotTag);
}

void UActionCombatAccessoryComponent::RequestClearAllAccessories()
{
    if (!HasAuthorityForAccessories())
    {
        ServerRequestClearAllAccessories();
        return;
    }

    HandleClearRequest();
}

TArray<FActionCombatEquippedAccessoryView> UActionCombatAccessoryComponent::GetEquippedAccessories() const
{
    TArray<FActionCombatEquippedAccessoryView> Result;
    Result.Reserve(EquippedAccessories.GetEntries().Num());

    for (const FActionCombatEquippedAccessoryEntry& Entry : EquippedAccessories.GetEntries())
    {
        FActionCombatEquippedAccessoryView View;
        View.SlotTag = Entry.SlotTag;
        View.AccessoryData = Entry.AccessoryData;
        Result.Add(View);
    }

    return Result;
}

UActionCombatAppearanceComponent* UActionCombatAccessoryComponent::GetAppearanceComponent() const
{
    return ResolveAppearanceComponent();
}

void UActionCombatAccessoryComponent::OnRep_AccessoryVisualSync()
{
    RefreshAccessoryVisuals();
    OnAccessoryVisualsChanged.Broadcast();
}

void UActionCombatAccessoryComponent::HandleReplicatedAccessoriesChanged()
{
    RefreshAccessoryVisuals();
    OnAccessoryVisualsChanged.Broadcast();
}

void UActionCombatAccessoryComponent::HandleReplicatedAccessoryRemoved(FGameplayTag SlotTag)
{
    DestroySpawnedAccessoryVisual(SlotTag);
    OnAccessoryVisualsChanged.Broadcast();
}

void UActionCombatAccessoryComponent::ServerRequestEquipAccessory_Implementation(FSoftObjectPath AccessoryDataPath)
{
    HandleEquipRequest(AccessoryDataPath);
}

void UActionCombatAccessoryComponent::ServerRequestUnequipSlot_Implementation(FGameplayTag SlotTag)
{
    HandleUnequipRequest(SlotTag);
}

void UActionCombatAccessoryComponent::ServerRequestClearAllAccessories_Implementation()
{
    HandleClearRequest();
}

bool UActionCombatAccessoryComponent::HandleEquipRequest(const FSoftObjectPath& AccessoryDataPath)
{
    UActionCombatAccessoryData* AccessoryData = LoadAccessoryData(AccessoryDataPath);
    if (!AccessoryData)
    {
        UE_LOG(LogActionCombatRuntime, Warning, TEXT("AccessoryComponent on %s failed to load accessory asset from path %s."), *GetPathNameSafe(GetOwner()), *AccessoryDataPath.ToString());
        return false;
    }

    FString FailureReason;
    if (!ValidateAccessoryData(AccessoryData, FailureReason))
    {
        UE_LOG(LogActionCombatRuntime, Warning, TEXT("AccessoryComponent rejected equip request on %s. %s"), *GetPathNameSafe(GetOwner()), *FailureReason);
        return false;
    }

    const TSoftObjectPtr<UActionCombatAccessoryData> AccessoryRef(AccessoryData);
    const bool bChanged = EquippedAccessories.SetEntry(AccessoryData->GetSlotTag(), AccessoryRef);
    if (!bChanged)
    {
        if (!SpawnedAccessoryVisuals.Contains(AccessoryData->GetSlotTag()))
        {
            RefreshAccessoryVisuals();
        }

        UE_LOG(LogActionCombatRuntime, Verbose, TEXT("AccessoryComponent on %s kept existing accessory %s for slot %s."),
            *GetPathNameSafe(GetOwner()),
            *GetPathNameSafe(AccessoryData),
            *AccessoryData->GetSlotTag().ToString());
        MarkAccessoryVisualStateDirty();
        return true;
    }

    RefreshAccessoryVisuals();
    MarkAccessoryVisualStateDirty();
    OnAccessoryVisualsChanged.Broadcast();
    UE_LOG(LogActionCombatRuntime, Log, TEXT("AccessoryComponent on %s equipped %s in slot %s."),
        *GetPathNameSafe(GetOwner()),
        *GetPathNameSafe(AccessoryData),
        *AccessoryData->GetSlotTag().ToString());
    return true;
}

bool UActionCombatAccessoryComponent::HandleUnequipRequest(const FGameplayTag& SlotTag)
{
    const bool bChanged = EquippedAccessories.RemoveEntry(SlotTag);
    if (!bChanged)
    {
        return false;
    }

    RefreshAccessoryVisuals();
    MarkAccessoryVisualStateDirty();
    OnAccessoryVisualsChanged.Broadcast();
    UE_LOG(LogActionCombatRuntime, Log, TEXT("AccessoryComponent on %s unequipped slot %s."),
        *GetPathNameSafe(GetOwner()),
        *SlotTag.ToString());
    return true;
}

void UActionCombatAccessoryComponent::HandleClearRequest()
{
    const bool bChanged = EquippedAccessories.ClearEntries();
    RefreshAccessoryVisuals();
    MarkAccessoryVisualStateDirty();
    OnAccessoryVisualsChanged.Broadcast();
    UE_LOG(LogActionCombatRuntime, Log, TEXT("AccessoryComponent on %s cleared all accessories. Changed=%s"),
        *GetPathNameSafe(GetOwner()),
        bChanged ? TEXT("true") : TEXT("false"));
}

void UActionCombatAccessoryComponent::MarkAccessoryVisualStateDirty()
{
    if (!HasAuthorityForAccessories())
    {
        return;
    }

    AuthoritativeAccessoryEntryCount = EquippedAccessories.GetEntries().Num();
    ++AccessoryVisualSyncRevision;

    if (AActor* Owner = GetOwner())
    {
        Owner->ForceNetUpdate();
    }
}

UActionCombatAccessoryData* UActionCombatAccessoryComponent::LoadAccessoryData(const FSoftObjectPath& AccessoryDataPath) const
{
    if (!AccessoryDataPath.IsValid())
    {
        return nullptr;
    }

    TSoftObjectPtr<UActionCombatAccessoryData> AccessoryDataPtr(AccessoryDataPath);
    return AccessoryDataPtr.LoadSynchronous();
}

UActionCombatAppearanceComponent* UActionCombatAccessoryComponent::ResolveAppearanceComponent() const
{
    if (AActor* Owner = GetOwner())
    {
        if (UActionCombatAppearanceComponent* ExplicitAppearance = Cast<UActionCombatAppearanceComponent>(AppearanceComponentReference.GetComponent(Owner)))
        {
            return ExplicitAppearance;
        }

        if (bAutoResolveAppearanceComponent)
        {
            return Owner->FindComponentByClass<UActionCombatAppearanceComponent>();
        }
    }

    return nullptr;
}

bool UActionCombatAccessoryComponent::ValidateAccessoryData(const UActionCombatAccessoryData* AccessoryData, FString& OutFailureReason) const
{
    if (!AccessoryData)
    {
        OutFailureReason = TEXT("AccessoryData was null.");
        return false;
    }

    if (!AccessoryData->GetSlotTag().IsValid())
    {
        OutFailureReason = FString::Printf(TEXT("Accessory %s does not define a valid slot tag."), *GetPathNameSafe(AccessoryData));
        return false;
    }

    if (!AccessoryData->HasVisualAsset())
    {
        OutFailureReason = FString::Printf(TEXT("Accessory %s does not define a visual asset."), *GetPathNameSafe(AccessoryData));
        return false;
    }

    const UActionCombatAppearanceComponent* AppearanceComponent = ResolveAppearanceComponent();
    if (!AppearanceComponent)
    {
        OutFailureReason = TEXT("No ActionCombatAppearanceComponent was found on the owner.");
        return false;
    }

    return AccessoryData->IsCompatibleWithAppearance(AppearanceComponent->GetAppearanceTags(), &OutFailureReason);
}

void UActionCombatAccessoryComponent::RefreshAccessoryVisuals()
{
    DestroyAllSpawnedAccessoryVisuals();

    if (!GetOwner() || GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    if (!HasAuthorityForAccessories() && AuthoritativeAccessoryEntryCount == 0)
    {
        return;
    }

    for (const FActionCombatEquippedAccessoryEntry& Entry : EquippedAccessories.GetEntries())
    {
        CreateVisualForEntry(Entry);
    }
}

void UActionCombatAccessoryComponent::DestroyAllSpawnedAccessoryVisuals()
{
    for (TPair<FGameplayTag, TObjectPtr<USceneComponent>>& Pair : SpawnedAccessoryVisuals)
    {
        if (USceneComponent* SpawnedVisual = Pair.Value)
        {
            SpawnedVisual->DestroyComponent();
        }
    }

    SpawnedAccessoryVisuals.Empty();
}

void UActionCombatAccessoryComponent::DestroySpawnedAccessoryVisual(FGameplayTag SlotTag)
{
    if (TObjectPtr<USceneComponent>* SpawnedVisualPtr = SpawnedAccessoryVisuals.Find(SlotTag))
    {
        if (USceneComponent* SpawnedVisual = SpawnedVisualPtr->Get())
        {
            SpawnedVisual->DestroyComponent();
        }

        SpawnedAccessoryVisuals.Remove(SlotTag);
    }
}

void UActionCombatAccessoryComponent::CreateVisualForEntry(const FActionCombatEquippedAccessoryEntry& Entry)
{
    DestroySpawnedAccessoryVisual(Entry.SlotTag);

    UActionCombatAccessoryData* AccessoryData = Entry.AccessoryData.LoadSynchronous();
    if (!AccessoryData)
    {
        UE_LOG(LogActionCombatRuntime, Warning, TEXT("AccessoryComponent on %s could not load equipped accessory for slot %s."), *GetPathNameSafe(GetOwner()), *Entry.SlotTag.ToString());
        return;
    }

    UActionCombatAppearanceComponent* AppearanceComponent = ResolveAppearanceComponent();
    if (!AppearanceComponent)
    {
        UE_LOG(LogActionCombatRuntime, Warning, TEXT("AccessoryComponent on %s has no appearance component to attach slot %s."), *GetPathNameSafe(GetOwner()), *Entry.SlotTag.ToString());
        return;
    }

    USceneComponent* AttachComponent = nullptr;
    FName AttachSocket = NAME_None;
    if (!AppearanceComponent->TryResolveAccessoryAttachment(Entry.SlotTag, AttachComponent, AttachSocket))
    {
        UE_LOG(LogActionCombatRuntime, Warning, TEXT("AccessoryComponent on %s could not resolve an attachment target for slot %s."), *GetPathNameSafe(GetOwner()), *Entry.SlotTag.ToString());
        return;
    }

    if (!AccessoryData->GetSocketOverride().IsNone())
    {
        AttachSocket = AccessoryData->GetSocketOverride();
    }

    USceneComponent* SpawnedComponent = nullptr;

    if (!AccessoryData->GetStaticMesh().IsNull())
    {
        if (UStaticMesh* StaticMesh = AccessoryData->GetStaticMesh().LoadSynchronous())
        {
            UStaticMeshComponent* StaticMeshComponent = NewObject<UStaticMeshComponent>(GetOwner());
            StaticMeshComponent->SetStaticMesh(StaticMesh);
            StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            StaticMeshComponent->SetGenerateOverlapEvents(false);
            StaticMeshComponent->SetCanEverAffectNavigation(false);
            StaticMeshComponent->SetMobility(EComponentMobility::Movable);
            StaticMeshComponent->SetHiddenInGame(false);
            StaticMeshComponent->SetVisibility(true, true);
            StaticMeshComponent->SetupAttachment(AttachComponent, AttachSocket);
            StaticMeshComponent->SetRelativeTransform(AccessoryData->GetRelativeTransform());
            StaticMeshComponent->RegisterComponent();
            ApplyMaterialOverrides(StaticMeshComponent, AccessoryData->GetMaterialOverrides());
            SpawnedComponent = StaticMeshComponent;
        }
    }
    else if (!AccessoryData->GetSkeletalMesh().IsNull())
    {
        if (USkeletalMesh* SkeletalMesh = AccessoryData->GetSkeletalMesh().LoadSynchronous())
        {
            USkeletalMeshComponent* SkeletalMeshComponent = NewObject<USkeletalMeshComponent>(GetOwner());
            SkeletalMeshComponent->SetSkeletalMesh(SkeletalMesh);
            SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            SkeletalMeshComponent->SetGenerateOverlapEvents(false);
            SkeletalMeshComponent->SetCanEverAffectNavigation(false);
            SkeletalMeshComponent->SetMobility(EComponentMobility::Movable);
            SkeletalMeshComponent->SetHiddenInGame(false);
            SkeletalMeshComponent->SetVisibility(true, true);
            SkeletalMeshComponent->SetupAttachment(AttachComponent, AttachSocket);
            SkeletalMeshComponent->SetRelativeTransform(AccessoryData->GetRelativeTransform());

            if (TSubclassOf<UAnimInstance> AnimClass = AccessoryData->GetAnimClass())
            {
                SkeletalMeshComponent->SetAnimInstanceClass(AnimClass);
            }

            SkeletalMeshComponent->RegisterComponent();
            ApplyMaterialOverrides(SkeletalMeshComponent, AccessoryData->GetMaterialOverrides());
            SpawnedComponent = SkeletalMeshComponent;
        }
    }

    if (!SpawnedComponent)
    {
        UE_LOG(LogActionCombatRuntime, Warning, TEXT("AccessoryComponent on %s failed to create a visual component for %s."), *GetPathNameSafe(GetOwner()), *GetPathNameSafe(AccessoryData));
        return;
    }

    SpawnedAccessoryVisuals.Add(Entry.SlotTag, SpawnedComponent);
    UE_LOG(LogActionCombatRuntime, Log, TEXT("AccessoryComponent on %s spawned visual %s for accessory %s. Slot=%s Attach=%s Socket=%s Relative=%s"),
        *GetPathNameSafe(GetOwner()),
        *GetPathNameSafe(SpawnedComponent),
        *GetPathNameSafe(AccessoryData),
        *Entry.SlotTag.ToString(),
        *GetPathNameSafe(AttachComponent),
        *AttachSocket.ToString(),
        *AccessoryData->GetRelativeTransform().ToHumanReadableString());
}

void UActionCombatAccessoryComponent::ApplyMaterialOverrides(UPrimitiveComponent* PrimitiveComponent, const TArray<TSoftObjectPtr<UMaterialInterface>>& MaterialOverrides) const
{
    if (!PrimitiveComponent)
    {
        return;
    }

    for (int32 MaterialIndex = 0; MaterialIndex < MaterialOverrides.Num(); ++MaterialIndex)
    {
        if (UMaterialInterface* Material = MaterialOverrides[MaterialIndex].LoadSynchronous())
        {
            PrimitiveComponent->SetMaterial(MaterialIndex, Material);
        }
    }
}

bool UActionCombatAccessoryComponent::HasAuthorityForAccessories() const
{
    return GetOwner() && GetOwner()->HasAuthority();
}
