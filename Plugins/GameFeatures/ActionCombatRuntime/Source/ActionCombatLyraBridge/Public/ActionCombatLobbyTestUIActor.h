#pragma once

#include "GameFramework/Actor.h"
#include "ActionCombatLobbyTestLoadoutWidget.h"

#include "ActionCombatLobbyTestUIActor.generated.h"

UCLASS()
class ACTIONCOMBATLYRABRIDGE_API AActionCombatLobbyTestUIActor : public AActor
{
    GENERATED_BODY()

public:
    AActionCombatLobbyTestUIActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void TryCreateLobbyUi();

    UPROPERTY(EditAnywhere, Category = "Lobby Cosmetics")
    TSubclassOf<UActionCombatLobbyTestLoadoutWidget> WidgetClass;

    UPROPERTY(EditAnywhere, Category = "Lobby Cosmetics")
    TArray<FActionCombatLobbyCosmeticSlot> CosmeticSlots;

    UPROPERTY(EditAnywhere, Category = "Lobby Cosmetics")
    TArray<FActionCombatLobbyCosmeticOption> CosmeticOptions;

    UPROPERTY(EditAnywhere, Category = "Lobby Cosmetics")
    FSoftObjectPath TargetExperiencePath;

    UPROPERTY(EditAnywhere, Category = "Lobby Cosmetics")
    bool bClearSlotsOnOpen = true;

    UPROPERTY(EditAnywhere, Category = "Lobby Cosmetics", meta = (ClampMin = "0"))
    int32 ViewportZOrder = 50;

    UPROPERTY(Transient)
    TObjectPtr<UActionCombatLobbyTestLoadoutWidget> ActiveWidget = nullptr;

    FTimerHandle CreateWidgetRetryTimerHandle;
};
