#pragma once

#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"

#include "ActionCombatLyraInputBridgeComponent.generated.h"

class APawn;
class UActionCombatComponent;
class UActionCombatLyraGuardComponent;
class UInputAction;
class UInputComponent;
class ULyraInputConfig;

USTRUCT(BlueprintType)
struct ACTIONCOMBATLYRABRIDGE_API FActionCombatLyraInputBinding
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Binding", meta = (Categories = "InputTag"))
    FGameplayTag InputTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Binding", meta = (Categories = "Combat.Command"))
    FGameplayTag StartedCommandTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Binding", meta = (Categories = "Combat.Command"))
    FGameplayTag CompletedCommandTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Binding", meta = (Categories = "Combat.Input.Held"))
    FGameplayTag HeldInputStateTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Binding")
    bool bMirrorHeldStateWhilePressed = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Binding")
    bool bRepeatStartedCommandWhileHeld = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Binding", meta = (ClampMin = "0.03"))
    float StartedCommandRepeatIntervalSeconds = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Binding")
    bool bSetFocusActiveOnStarted = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Binding")
    bool bSetFocusInactiveOnCompleted = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Binding")
    bool bSetGuardInputHeldOnStarted = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Binding")
    bool bClearGuardInputHeldOnCompleted = false;

    bool WantsStartedBinding() const
    {
        return StartedCommandTag.IsValid()
            || (bMirrorHeldStateWhilePressed && HeldInputStateTag.IsValid())
            || bSetFocusActiveOnStarted
            || bSetGuardInputHeldOnStarted;
    }

    bool WantsCompletedBinding() const
    {
        return CompletedCommandTag.IsValid()
            || (bMirrorHeldStateWhilePressed && HeldInputStateTag.IsValid())
            || bSetFocusInactiveOnCompleted
            || bClearGuardInputHeldOnCompleted;
    }

    bool WantsTriggeredBinding() const
    {
        return bRepeatStartedCommandWhileHeld && (StartedCommandTag.IsValid() || InputTag.IsValid());
    }
};

UCLASS(BlueprintType, ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class ACTIONCOMBATLYRABRIDGE_API UActionCombatLyraInputBridgeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UActionCombatLyraInputBridgeComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "Action Combat|Lyra Bridge")
    void RefreshInputBindings();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Lyra Bridge")
    FComponentReference ActionCombatComponentReference;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Lyra Bridge")
    FComponentReference GuardComponentReference;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Lyra Bridge")
    bool bBindOnlyLocallyControlled = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Lyra Bridge")
    bool bLogBindingFlow = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Lyra Bridge")
    bool bSearchAbilityInputActionsAsFallback = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Lyra Bridge")
    bool bPollRightMouseButtonForSecondary = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Combat|Lyra Bridge")
    TArray<FActionCombatLyraInputBinding> InputBindings;

private:
    void TryBindInput();
    void RemoveInputBindings();
    void HandleInputStarted(FGameplayTag InputTag);
    void HandleInputTriggered(FGameplayTag InputTag);
    void HandleInputCompleted(FGameplayTag InputTag);
    void BindRawSecondaryKeyInput(UInputComponent* InputComponent);
    void UnbindRawSecondaryKeyInput();
    void HandleRawSecondaryPressed();
    void HandleRawSecondaryReleased();
    void PollRawSecondaryInput();
    void ProcessRawHeldRepeat(FGameplayTag InputTag);
    void ApplyStartedBinding(const FActionCombatLyraInputBinding& Binding);
    void ApplyRepeatedStartedCommand(const FActionCombatLyraInputBinding& Binding);
    void ApplyCompletedBinding(const FActionCombatLyraInputBinding& Binding);
    void LogBinding(const FString& Message) const;
    APawn* ResolvePawnOwner() const;
    UActionCombatComponent* ResolveActionCombatComponent() const;
    UActionCombatLyraGuardComponent* ResolveGuardComponent() const;
    const ULyraInputConfig* ResolveInputConfig() const;
    const UInputAction* ResolveInputActionForTag(const ULyraInputConfig* InputConfig, const FGameplayTag& InputTag) const;
    const FActionCombatLyraInputBinding* FindBindingByInputTag(const FGameplayTag& InputTag) const;

    TWeakObjectPtr<UInputComponent> BoundInputComponent;
    TArray<uint32> BoundInputHandles;
    TMap<FGameplayTag, double> NextRepeatCommandTimeByInputTag;
    TMap<FGameplayTag, double> LastStartedInputTimeByInputTag;

    struct FRawSecondaryKeyBinding
    {
        TWeakObjectPtr<UInputComponent> InputComponent;
        int32 PressedBindingIndex = INDEX_NONE;
        int32 ReleasedBindingIndex = INDEX_NONE;
    };

    TArray<FRawSecondaryKeyBinding> RawSecondaryKeyBindings;
    bool bWasRawSecondaryInputDown = false;
};
