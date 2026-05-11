#pragma once

#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "Lobby/LyraLobbyTypes.h"

#include "ActionCombatLobbyCosmeticBridgeComponent.generated.h"

class UActionCombatAccessoryComponent;
class UActionCombatAccessoryData;
class ULyraLobbyPlayerStateComponent;

UCLASS(BlueprintType, ClassGroup = (Cosmetics), meta = (BlueprintSpawnableComponent))
class ACTIONCOMBATLYRABRIDGE_API UActionCombatLobbyCosmeticBridgeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UActionCombatLobbyCosmeticBridgeComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Action Combat|Lobby Cosmetics")
    bool ApplyLobbyCosmeticsNow();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Lobby Cosmetics")
    FComponentReference AccessoryComponentReference;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Lobby Cosmetics")
    bool bApplyOnBeginPlay = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Lobby Cosmetics")
    bool bClearExistingAccessoriesBeforeApply = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Lobby Cosmetics", meta = (ClampMin = "1"))
    int32 MaxApplyAttempts = 120;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Lobby Cosmetics", meta = (ClampMin = "0.01"))
    float RetryIntervalSeconds = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Lobby Cosmetics")
    bool bLogFlow = true;

private:
    UActionCombatAccessoryComponent* ResolveAccessoryComponent() const;
    ULyraLobbyPlayerStateComponent* ResolveLobbyPlayerStateComponent() const;
    UActionCombatAccessoryData* ResolveAccessoryData(const FLyraLobbyAccessorySelection& Selection) const;
    bool ApplyLoadout(const FLyraLobbyPlayerLoadout& Loadout);
    void ScheduleRetryIfNeeded();
    void RetryApplyLobbyCosmetics();
    void BindLobbyLoadoutChanged(ULyraLobbyPlayerStateComponent* LobbyPlayer);
    void UnbindLobbyLoadoutChanged();
    void LogFlow(const FString& Message) const;

    UFUNCTION()
    void HandleLobbyLoadoutChanged(const FLyraLobbyPlayerLoadout& LobbyLoadout);

    FTimerHandle RetryTimerHandle;
    int32 ApplyAttempts = 0;
    int32 LastAppliedRevision = INDEX_NONE;
    TWeakObjectPtr<ULyraLobbyPlayerStateComponent> BoundLobbyPlayerStateComponent;
};
