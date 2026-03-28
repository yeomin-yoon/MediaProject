#pragma once

#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"

#include "ActionCombatComponent.h"
#include "ActionCombatLyraAbilityBridgeComponent.generated.h"

class APawn;
class UActionCombatComponent;
class ULyraAbilitySystemComponent;

USTRUCT(BlueprintType)
struct ACTIONCOMBATLYRABRIDGE_API FActionCombatLyraActionEventBinding
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Binding", meta = (Categories = "Combat.Action"))
    FGameplayTag ActionTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Binding")
    FGameplayTag StartedEventTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Binding")
    FGameplayTag EndedEventTag;
};

UCLASS(BlueprintType, ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class ACTIONCOMBATLYRABRIDGE_API UActionCombatLyraAbilityBridgeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UActionCombatLyraAbilityBridgeComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Lyra Bridge")
    void RefreshCombatBinding();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Lyra Bridge")
    FComponentReference ActionCombatComponentReference;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Lyra Bridge")
    bool bDispatchEventsOnlyOnAuthority = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Lyra Bridge")
    bool bDispatchActionStartedEvents = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Lyra Bridge")
    bool bDispatchActionEndedEvents = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Lyra Bridge")
    bool bLogAbilityBridge = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Lyra Bridge")
    FGameplayTag DefaultActionStartedEventTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Lyra Bridge")
    FGameplayTag DefaultActionEndedEventTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Lyra Bridge")
    TArray<FActionCombatLyraActionEventBinding> ActionEventBindings;

private:
    void TryBindCombatComponent();
    void UnbindCombatComponent();
    void DispatchEventForActionState(const FGameplayTag& EventTag, const FActionCombatActiveActionState& ActionState, bool bIsEndEvent);
    void LogBridge(const FString& Message) const;
    APawn* ResolvePawnOwner() const;
    UActionCombatComponent* ResolveActionCombatComponent() const;
    ULyraAbilitySystemComponent* ResolveLyraAbilitySystemComponent() const;
    const FActionCombatLyraActionEventBinding* FindActionEventBinding(const FGameplayTag& ActionTag) const;
    FGameplayTag ResolveStartedEventTag(const FGameplayTag& ActionTag) const;
    FGameplayTag ResolveEndedEventTag(const FGameplayTag& ActionTag) const;

    UFUNCTION()
    void HandleActionStarted(FActionCombatActiveActionState ActionState);

    UFUNCTION()
    void HandleActionEnded(FActionCombatActiveActionState ActionState);

    TWeakObjectPtr<UActionCombatComponent> BoundCombatComponent;
};
