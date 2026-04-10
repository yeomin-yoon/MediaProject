#pragma once

#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/SoftObjectPtr.h"

#include "ActionCombatAccessoryComponent.generated.h"

class UActionCombatAccessoryComponent;
class UActionCombatAccessoryData;
class UActionCombatAppearanceComponent;
class UMaterialInterface;
class UPrimitiveComponent;
class USceneComponent;

USTRUCT(BlueprintType)
struct ACTIONCOMBATRUNTIME_API FActionCombatEquippedAccessoryView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Accessories")
    FGameplayTag SlotTag;

    UPROPERTY(BlueprintReadOnly, Category = "Accessories")
    TSoftObjectPtr<UActionCombatAccessoryData> AccessoryData;

    bool IsEquipped() const
    {
        return SlotTag.IsValid() && AccessoryData.ToSoftObjectPath().IsValid();
    }
};

USTRUCT()
struct ACTIONCOMBATRUNTIME_API FActionCombatEquippedAccessoryEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag SlotTag;

    UPROPERTY()
    TSoftObjectPtr<UActionCombatAccessoryData> AccessoryData;
};

USTRUCT()
struct ACTIONCOMBATRUNTIME_API FActionCombatEquippedAccessoryList : public FFastArraySerializer
{
    GENERATED_BODY()

public:
    void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
    void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FActionCombatEquippedAccessoryEntry, FActionCombatEquippedAccessoryList>(Entries, DeltaParms, *this);
    }

    void SetOwnerComponent(UActionCombatAccessoryComponent* InOwnerComponent)
    {
        OwnerComponent = InOwnerComponent;
    }

    bool SetEntry(FGameplayTag SlotTag, const TSoftObjectPtr<UActionCombatAccessoryData>& AccessoryData);
    bool RemoveEntry(FGameplayTag SlotTag);
    bool ClearEntries();

    const TArray<FActionCombatEquippedAccessoryEntry>& GetEntries() const
    {
        return Entries;
    }

private:
    UPROPERTY()
    TArray<FActionCombatEquippedAccessoryEntry> Entries;

    UPROPERTY(NotReplicated, Transient)
    TObjectPtr<UActionCombatAccessoryComponent> OwnerComponent = nullptr;
};

template<>
struct TStructOpsTypeTraits<FActionCombatEquippedAccessoryList> : public TStructOpsTypeTraitsBase2<FActionCombatEquippedAccessoryList>
{
    enum
    {
        WithNetDeltaSerializer = true
    };
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FActionCombatAccessoryVisualsChangedSignature);

UCLASS(BlueprintType, ClassGroup = (Cosmetics), meta = (BlueprintSpawnableComponent))
class ACTIONCOMBATRUNTIME_API UActionCombatAccessoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UActionCombatAccessoryComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void OnRegister() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Accessories")
    bool RequestEquipAccessory(UActionCombatAccessoryData* AccessoryData);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Accessories")
    bool RequestUnequipSlot(FGameplayTag SlotTag);

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Accessories")
    void RequestClearAllAccessories();

    UFUNCTION(BlueprintPure, Category = "Action Combat|Accessories")
    TArray<FActionCombatEquippedAccessoryView> GetEquippedAccessories() const;

    UFUNCTION(BlueprintPure, Category = "Action Combat|Accessories")
    UActionCombatAppearanceComponent* GetAppearanceComponent() const;

    void HandleReplicatedAccessoriesChanged();
    void HandleReplicatedAccessoryRemoved(FGameplayTag SlotTag);

    UPROPERTY(BlueprintAssignable, Category = "Action Combat|Accessories")
    FActionCombatAccessoryVisualsChangedSignature OnAccessoryVisualsChanged;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Accessories")
    FComponentReference AppearanceComponentReference;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Accessories")
    bool bAutoResolveAppearanceComponent = true;

    UPROPERTY(Replicated)
    FActionCombatEquippedAccessoryList EquippedAccessories;

private:
    UFUNCTION(Server, Reliable)
    void ServerRequestEquipAccessory(FSoftObjectPath AccessoryDataPath);

    UFUNCTION(Server, Reliable)
    void ServerRequestUnequipSlot(FGameplayTag SlotTag);

    UFUNCTION(Server, Reliable)
    void ServerRequestClearAllAccessories();

    bool HandleEquipRequest(const FSoftObjectPath& AccessoryDataPath);
    bool HandleUnequipRequest(const FGameplayTag& SlotTag);
    void HandleClearRequest();

    UActionCombatAccessoryData* LoadAccessoryData(const FSoftObjectPath& AccessoryDataPath) const;
    UActionCombatAppearanceComponent* ResolveAppearanceComponent() const;
    bool ValidateAccessoryData(const UActionCombatAccessoryData* AccessoryData, FString& OutFailureReason) const;
    void RefreshAccessoryVisuals();
    void DestroySpawnedAccessoryVisual(FGameplayTag SlotTag);
    void DestroyAllSpawnedAccessoryVisuals();
    void CreateVisualForEntry(const FActionCombatEquippedAccessoryEntry& Entry);
    void ApplyMaterialOverrides(UPrimitiveComponent* PrimitiveComponent, const TArray<TSoftObjectPtr<UMaterialInterface>>& MaterialOverrides) const;
    bool HasAuthorityForAccessories() const;

    UPROPERTY(Transient)
    TMap<FGameplayTag, TObjectPtr<USceneComponent>> SpawnedAccessoryVisuals;
};
