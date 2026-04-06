#pragma once

#include "Components/ActorComponent.h"
#include "Delegates/Delegate.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPtr.h"

#include "ActionCombatComponent.h"
#include "ActionCombatLyraAbilityBridgeComponent.generated.h"

class APawn;
class UGameplayAbility;
class UActionCombatComponent;
class ULyraPawnExtensionComponent;
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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Dash Dodge")
    bool bMirrorDashAbilityToCombatState = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Dash Dodge")
    bool bMirrorDashAbilityToIFrameState = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Dash Dodge")
    bool bMirrorDashStateOnlyOnAuthority = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Dash Dodge")
    bool bInterruptActiveCombatActionOnDashStart = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Dash Dodge")
    bool bGrantFallbackDashAbilityOnServerIfMissing = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Dash Dodge", meta = (Categories = "Ability.Type"))
    FGameplayTag ObservedDashAbilityTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Dash Dodge", meta = (Categories = "Combat.State"))
    FGameplayTag GrantedDashStateTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Dash Dodge", meta = (Categories = "Combat.State"))
    FGameplayTag GrantedDashIFrameStateTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Dash Dodge")
    TSoftClassPtr<UGameplayAbility> FallbackDashAbilityClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Dash Dodge", meta = (Categories = "InputTag"))
    FGameplayTag FallbackDashInputTag;

private:
    void BindPawnExtension();
    void TryBindCombatComponent();
    void UnbindCombatComponent();
    void BindLyraAbilitySystemComponent(ULyraAbilitySystemComponent* AbilitySystemComponent);
    void UnbindLyraAbilitySystemComponent();
    void EnsureFallbackDashAbilityGranted();
    bool HasObservedDashAbilitySpec(const ULyraAbilitySystemComponent& AbilitySystemComponent) const;
    void RefreshMirroredDashStateFromAbilitySystem();
    void SetMirroredDashStateActive(bool bNewDashStateActive);
    bool IsObservedDashAbility(const UGameplayAbility* Ability) const;
    void DispatchEventForActionState(const FGameplayTag& EventTag, const FActionCombatActiveActionState& ActionState, bool bIsEndEvent);
    void LogBridge(const FString& Message) const;
    APawn* ResolvePawnOwner() const;
    ULyraPawnExtensionComponent* ResolvePawnExtensionComponent() const;
    UActionCombatComponent* ResolveActionCombatComponent() const;
    ULyraAbilitySystemComponent* ResolveLyraAbilitySystemComponent() const;
    const FActionCombatLyraActionEventBinding* FindActionEventBinding(const FGameplayTag& ActionTag) const;
    FGameplayTag ResolveStartedEventTag(const FGameplayTag& ActionTag) const;
    FGameplayTag ResolveEndedEventTag(const FGameplayTag& ActionTag) const;

    UFUNCTION()
    void HandleActionStarted(FActionCombatActiveActionState ActionState);

    UFUNCTION()
    void HandleActionEnded(FActionCombatActiveActionState ActionState);

    UFUNCTION()
    void HandleObservedDashAbilityTagChanged(const FGameplayTag ChangedTag, int32 NewCount);

    void HandleObservedAbilityActivated(UGameplayAbility* Ability);

    void HandleObservedAbilityEnded(UGameplayAbility* Ability);

    UFUNCTION()
    void HandleAbilitySystemInitialized();

    UFUNCTION()
    void HandleAbilitySystemUninitialized();

    TWeakObjectPtr<UActionCombatComponent> BoundCombatComponent;
    TWeakObjectPtr<ULyraAbilitySystemComponent> BoundAbilitySystemComponent;
    FDelegateHandle DashAbilityTagChangedHandle;
    FDelegateHandle DashAbilityActivatedHandle;
    FDelegateHandle DashAbilityEndedHandle;
    bool bMirroredDashStateActive = false;
    bool bObservedDashAbilityTagActive = false;
    int32 ActiveObservedDashAbilityCount = 0;
};
