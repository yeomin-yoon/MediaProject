#include "ActionCombatReactionComponent.h"

#include "ActionCombatComponent.h"
#include "ActionCombatReactionSet.h"
#include "ActionCombatRuntimeLog.h"
#include "ActionCombatRuntimeTags.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

const FActionCombatReactionAnimation* FActionCombatDirectionalReactionAnimations::FindAnimation(EActionCombatReactionDirection Direction) const
{
    const FActionCombatReactionAnimation* ExactMatch = nullptr;
    switch (Direction)
    {
    case EActionCombatReactionDirection::Front:
        ExactMatch = &Front;
        break;
    case EActionCombatReactionDirection::Back:
        ExactMatch = &Back;
        break;
    case EActionCombatReactionDirection::Left:
        ExactMatch = &Left;
        break;
    case EActionCombatReactionDirection::Right:
        ExactMatch = &Right;
        break;
    default:
        break;
    }

    if (ExactMatch && ExactMatch->HasAnimation())
    {
        return ExactMatch;
    }

    const FActionCombatReactionAnimation* Fallbacks[] = { &Front, &Back, &Left, &Right };
    for (const FActionCombatReactionAnimation* Fallback : Fallbacks)
    {
        if (Fallback && Fallback->HasAnimation())
        {
            return Fallback;
        }
    }

    return nullptr;
}

UActionCombatReactionComponent::UActionCombatReactionComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(true);

    LightHitAnimations.Front.Montage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Characters/Heroes/Mannequin/Animations/Actions/AM_MM_HitReact_Front_Lgt_01.AM_MM_HitReact_Front_Lgt_01")));
    LightHitAnimations.Back.Montage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Characters/Heroes/Mannequin/Animations/Actions/AM_MM_HitReact_Back_Lgt_01.AM_MM_HitReact_Back_Lgt_01")));
    LightHitAnimations.Left.Montage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Characters/Heroes/Mannequin/Animations/Actions/AM_MM_HitReact_Left_Lgt_01.AM_MM_HitReact_Left_Lgt_01")));
    LightHitAnimations.Right.Montage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Characters/Heroes/Mannequin/Animations/Actions/AM_MM_HitReact_Right_Lgt_01.AM_MM_HitReact_Right_Lgt_01")));

    HeavyHitAnimations.Front.Montage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Characters/Heroes/Mannequin/Animations/Actions/AM_MM_HitReact_Front_Hvy_01.AM_MM_HitReact_Front_Hvy_01")));
    HeavyHitAnimations.Back.Montage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Characters/Heroes/Mannequin/Animations/Actions/AM_MM_HitReact_Back_Med_01.AM_MM_HitReact_Back_Med_01")));
    HeavyHitAnimations.Left.Montage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Characters/Heroes/Mannequin/Animations/Actions/AM_MM_HitReact_Left_Med_01.AM_MM_HitReact_Left_Med_01")));
    HeavyHitAnimations.Right.Montage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Characters/Heroes/Mannequin/Animations/Actions/AM_MM_HitReact_Right_Med_01.AM_MM_HitReact_Right_Med_01")));

    KnockdownAnimation.Sequence = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/1dev/OS/QuaterniusUAL2/Retargeted/Manny/Manny_Hit_Knockback_RM.Manny_Hit_Knockback_RM")));
    KnockdownAnimation.BlendInSeconds = 0.08f;
    KnockdownAnimation.BlendOutSeconds = 0.12f;
}

void UActionCombatReactionComponent::BeginPlay()
{
    Super::BeginPlay();
    EnsureReactionSet();
}

void UActionCombatReactionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!HasReactionAuthority() || IsInReactionState())
    {
        return;
    }

    const double CurrentWorldTimeSeconds = GetCurrentWorldTimeSeconds();
    if ((LastIncomingHitWorldTimeSeconds >= 0.0) && ((CurrentWorldTimeSeconds - LastIncomingHitWorldTimeSeconds) < PoiseRecoveryDelaySeconds))
    {
        return;
    }

    if (EnsureReactionSet() == nullptr)
    {
        SetComponentTickEnabled(false);
        return;
    }

    const float MaxPoise = GetMaxPoise();
    const float CurrentPoise = GetCurrentPoise();
    if (CurrentPoise >= (MaxPoise - KINDA_SMALL_NUMBER))
    {
        SetCurrentPoise(MaxPoise);
        SetComponentTickEnabled(false);
        return;
    }

    const float NewPoise = FMath::Min(CurrentPoise + (GetPoiseRecoveryRate() * DeltaTime), MaxPoise);
    SetCurrentPoise(NewPoise);

    if (NewPoise >= (MaxPoise - KINDA_SMALL_NUMBER))
    {
        SetComponentTickEnabled(false);
    }
}

void UActionCombatReactionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, ReplicatedReactionCueState);
    DOREPLIFETIME(ThisClass, ReplicatedReactionCueDirection);
    DOREPLIFETIME(ThisClass, ReplicatedReactionCueId);
}

UActionCombatReactionComponent* UActionCombatReactionComponent::FindReactionComponent(const AActor* Actor)
{
    return Actor ? Actor->FindComponentByClass<UActionCombatReactionComponent>() : nullptr;
}

UActionCombatReactionComponent* UActionCombatReactionComponent::FindOrCreateReactionComponent(AActor* Actor)
{
    if (Actor == nullptr)
    {
        return nullptr;
    }

    if (UActionCombatReactionComponent* ExistingComponent = FindReactionComponent(Actor))
    {
        return ExistingComponent;
    }

    if (!Actor->HasAuthority())
    {
        return nullptr;
    }

    UActionCombatReactionComponent* NewComponent = NewObject<UActionCombatReactionComponent>(Actor);
    if (NewComponent == nullptr)
    {
        return nullptr;
    }

    NewComponent->SetIsReplicated(true);
    Actor->AddInstanceComponent(NewComponent);
    NewComponent->RegisterComponent();
    return NewComponent;
}

bool UActionCombatReactionComponent::HasReactionAuthority() const
{
    return GetOwner() && GetOwner()->HasAuthority();
}

EActionCombatReactionOutcome UActionCombatReactionComponent::GetActiveReactionOutcome() const
{
    switch (ActiveReactionState)
    {
    case EActionCombatReactionState::LightHit:
        return EActionCombatReactionOutcome::LightHit;
    case EActionCombatReactionState::HeavyHit:
        return EActionCombatReactionOutcome::HeavyHit;
    case EActionCombatReactionState::Knockdown:
        return EActionCombatReactionOutcome::Knockdown;
    default:
        return EActionCombatReactionOutcome::None;
    }
}

bool UActionCombatReactionComponent::IsInReactionState() const
{
    return ActiveReactionState != EActionCombatReactionState::None;
}

bool UActionCombatReactionComponent::TryApplyReactionHit(const FActionCombatReactionHit& IncomingHit, FActionCombatReactionResult& OutResult)
{
    OutResult = FActionCombatReactionResult();

    if (!HasReactionAuthority() || !IncomingHit.HasMeaningfulReaction())
    {
        return false;
    }

    const double CurrentWorldTimeSeconds = GetCurrentWorldTimeSeconds();
    if ((ReactionImmunityEndWorldTimeSeconds >= 0.0) && (CurrentWorldTimeSeconds < ReactionImmunityEndWorldTimeSeconds))
    {
        return false;
    }

    if ((ActiveReactionState == EActionCombatReactionState::Knockdown) || (ActiveReactionState == EActionCombatReactionState::GetUp))
    {
        return false;
    }

    UActionCombatReactionSet* ReactionSet = EnsureReactionSet();
    if (ReactionSet == nullptr)
    {
        return false;
    }

    const float MaxPoise = GetMaxPoise();
    const float EffectivePoiseDamage = FMath::Max(IncomingHit.PoiseDamage, 0.0f);
    const float EffectiveKnockdownPower = FMath::Max(IncomingHit.KnockdownPower, 0.0f);

    float PoiseBefore = FMath::Clamp(GetCurrentPoise(), 0.0f, MaxPoise);
    if (PoiseBefore <= KINDA_SMALL_NUMBER)
    {
        PoiseBefore = MaxPoise;
        SetCurrentPoise(PoiseBefore);
    }

    const float PoiseAfter = FMath::Clamp(PoiseBefore - EffectivePoiseDamage, 0.0f, MaxPoise);
    const bool bPoiseBroken = (EffectivePoiseDamage > KINDA_SMALL_NUMBER) && (PoiseAfter <= KINDA_SMALL_NUMBER);

    if ((ActiveReactionState != EActionCombatReactionState::None) && !bPoiseBroken && (EffectiveKnockdownPower < GetKnockdownThreshold()))
    {
        SetCurrentPoise(PoiseAfter);
        LastIncomingHitWorldTimeSeconds = CurrentWorldTimeSeconds;
        SetComponentTickEnabled(true);
        return false;
    }

    SetCurrentPoise(PoiseAfter);
    LastIncomingHitWorldTimeSeconds = CurrentWorldTimeSeconds;
    SetComponentTickEnabled(true);

    if ((CurrentWorldTimeSeconds - LastPoiseBreakWorldTimeSeconds) > PoiseBreakChainResetSeconds)
    {
        RecentPoiseBreakCount = 0;
    }

    bool bShouldKnockdown = EffectiveKnockdownPower >= GetKnockdownThreshold();
    if (bPoiseBroken)
    {
        LastPoiseBreakWorldTimeSeconds = CurrentWorldTimeSeconds;
        ++RecentPoiseBreakCount;
        bShouldKnockdown |= RecentPoiseBreakCount >= FMath::Max(PoiseBreaksBeforeKnockdown, 1);
    }

    OutResult.PoiseBefore = PoiseBefore;
    OutResult.PoiseAfter = PoiseAfter;
    OutResult.PoiseBreakChainCount = RecentPoiseBreakCount;

    if (bShouldKnockdown)
    {
        OutResult.Outcome = EActionCombatReactionOutcome::Knockdown;
        OutResult.bInterruptedCombatAction = InterruptCombatAction();
        RecentPoiseBreakCount = 0;
        SetCurrentPoise(MaxPoise);
        BeginKnockdown(IncomingHit.WorldSpaceImpulseDirection);
    }
    else if (bPoiseBroken)
    {
        OutResult.Outcome = EActionCombatReactionOutcome::HeavyHit;
        OutResult.bInterruptedCombatAction = InterruptCombatAction();
        SetCurrentPoise(MaxPoise);
        BeginTimedReaction(EActionCombatReactionState::HeavyHit, HeavyHitDurationSeconds, IncomingHit.WorldSpaceImpulseDirection);
    }
    else if (EffectivePoiseDamage > KINDA_SMALL_NUMBER)
    {
        OutResult.Outcome = EActionCombatReactionOutcome::LightHit;
        OutResult.bInterruptedCombatAction = InterruptCombatAction();
        BeginTimedReaction(EActionCombatReactionState::LightHit, LightHitDurationSeconds, IncomingHit.WorldSpaceImpulseDirection);
    }

    UE_LOG(
        LogActionCombatRuntime,
        Log,
        TEXT("[Reaction:%s] Outcome=%d PoiseBefore=%.2f PoiseAfter=%.2f PoiseDamage=%.2f KnockdownPower=%.2f BreakChain=%d"),
        *GetPathNameSafe(GetOwner()),
        static_cast<int32>(OutResult.Outcome),
        OutResult.PoiseBefore,
        OutResult.PoiseAfter,
        EffectivePoiseDamage,
        EffectiveKnockdownPower,
        OutResult.PoiseBreakChainCount);

    return OutResult.Outcome != EActionCombatReactionOutcome::None;
}

UAbilitySystemComponent* UActionCombatReactionComponent::ResolveAbilitySystemComponent() const
{
    return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
}

UActionCombatReactionSet* UActionCombatReactionComponent::FindReactionSet() const
{
    if (CachedReactionSet.IsValid())
    {
        return CachedReactionSet.Get();
    }

    if (const UAbilitySystemComponent* AbilitySystemComponent = ResolveAbilitySystemComponent())
    {
        for (const UAttributeSet* SpawnedSet : AbilitySystemComponent->GetSpawnedAttributes())
        {
            if (const UActionCombatReactionSet* ReactionSet = Cast<UActionCombatReactionSet>(SpawnedSet))
            {
                CachedReactionSet = const_cast<UActionCombatReactionSet*>(ReactionSet);
                return CachedReactionSet.Get();
            }
        }
    }

    return nullptr;
}

UActionCombatReactionSet* UActionCombatReactionComponent::EnsureReactionSet()
{
    if (UActionCombatReactionSet* ExistingSet = FindReactionSet())
    {
        CachedReactionSet = ExistingSet;
        return ExistingSet;
    }

    UAbilitySystemComponent* AbilitySystemComponent = ResolveAbilitySystemComponent();
    if ((AbilitySystemComponent == nullptr) || !HasReactionAuthority())
    {
        return nullptr;
    }

    UActionCombatReactionSet* NewSet = NewObject<UActionCombatReactionSet>(AbilitySystemComponent->GetOwner());
    if (NewSet == nullptr)
    {
        return nullptr;
    }

    AbilitySystemComponent->AddAttributeSetSubobject(NewSet);
    CachedReactionSet = NewSet;
    SetCurrentPoise(GetMaxPoise());
    return NewSet;
}

float UActionCombatReactionComponent::GetCurrentPoise() const
{
    if (const UActionCombatReactionSet* ReactionSet = FindReactionSet())
    {
        return ReactionSet->GetPoise();
    }

    return 0.0f;
}

float UActionCombatReactionComponent::GetMaxPoise() const
{
    if (const UActionCombatReactionSet* ReactionSet = FindReactionSet())
    {
        return FMath::Max(ReactionSet->GetMaxPoise(), 1.0f);
    }

    return 100.0f;
}

float UActionCombatReactionComponent::GetPoiseRecoveryRate() const
{
    if (const UActionCombatReactionSet* ReactionSet = FindReactionSet())
    {
        return FMath::Max(ReactionSet->GetPoiseRecoveryRate(), 0.0f);
    }

    return 0.0f;
}

float UActionCombatReactionComponent::GetKnockdownThreshold() const
{
    if (const UActionCombatReactionSet* ReactionSet = FindReactionSet())
    {
        return FMath::Max(ReactionSet->GetKnockdownThreshold(), 0.0f);
    }

    return 0.0f;
}

void UActionCombatReactionComponent::SetCurrentPoise(float NewValue) const
{
    if (UAbilitySystemComponent* AbilitySystemComponent = ResolveAbilitySystemComponent())
    {
        AbilitySystemComponent->SetNumericAttributeBase(UActionCombatReactionSet::GetPoiseAttribute(), FMath::Clamp(NewValue, 0.0f, GetMaxPoise()));
    }
}

void UActionCombatReactionComponent::ClearReactionTags() const
{
    if (UAbilitySystemComponent* AbilitySystemComponent = ResolveAbilitySystemComponent())
    {
        AbilitySystemComponent->SetLooseGameplayTagCount(ActionCombatRuntimeTags::Combat_State_Reaction, 0);
        AbilitySystemComponent->SetLooseGameplayTagCount(ActionCombatRuntimeTags::Combat_State_Reaction_LightHit, 0);
        AbilitySystemComponent->SetLooseGameplayTagCount(ActionCombatRuntimeTags::Combat_State_Reaction_HeavyHit, 0);
        AbilitySystemComponent->SetLooseGameplayTagCount(ActionCombatRuntimeTags::Combat_State_Reaction_Knockdown, 0);
        AbilitySystemComponent->SetLooseGameplayTagCount(ActionCombatRuntimeTags::Combat_State_Reaction_GetUp, 0);
    }
}

void UActionCombatReactionComponent::UpdateReactionTags() const
{
    ClearReactionTags();

    if (ActiveReactionState == EActionCombatReactionState::None)
    {
        return;
    }

    if (UAbilitySystemComponent* AbilitySystemComponent = ResolveAbilitySystemComponent())
    {
        AbilitySystemComponent->SetLooseGameplayTagCount(ActionCombatRuntimeTags::Combat_State_Reaction, 1);

        switch (ActiveReactionState)
        {
        case EActionCombatReactionState::LightHit:
            AbilitySystemComponent->SetLooseGameplayTagCount(ActionCombatRuntimeTags::Combat_State_Reaction_LightHit, 1);
            break;
        case EActionCombatReactionState::HeavyHit:
            AbilitySystemComponent->SetLooseGameplayTagCount(ActionCombatRuntimeTags::Combat_State_Reaction_HeavyHit, 1);
            break;
        case EActionCombatReactionState::Knockdown:
            AbilitySystemComponent->SetLooseGameplayTagCount(ActionCombatRuntimeTags::Combat_State_Reaction_Knockdown, 1);
            break;
        case EActionCombatReactionState::GetUp:
            AbilitySystemComponent->SetLooseGameplayTagCount(ActionCombatRuntimeTags::Combat_State_Reaction_GetUp, 1);
            break;
        default:
            break;
        }
    }
}

void UActionCombatReactionComponent::BeginTimedReaction(EActionCombatReactionState NewState, float DurationSeconds, const FVector& WorldSpaceImpulseDirection)
{
    if (GetWorld() == nullptr)
    {
        return;
    }

    GetWorld()->GetTimerManager().ClearTimer(ReactionTimerHandle);
    ActiveReactionState = NewState;
    LastReactionImpulseDirection = WorldSpaceImpulseDirection;
    UpdateReactionTags();
    StartReactionCue(NewState, WorldSpaceImpulseDirection);
    ApplyMovementLock(bDisableMovementDuringHitReaction);
    GetWorld()->GetTimerManager().SetTimer(ReactionTimerHandle, this, &ThisClass::HandleReactionTimerExpired, FMath::Max(DurationSeconds, 0.01f), false);
}

void UActionCombatReactionComponent::BeginKnockdown(const FVector& WorldSpaceImpulseDirection)
{
    if (GetWorld() == nullptr)
    {
        return;
    }

    GetWorld()->GetTimerManager().ClearTimer(ReactionTimerHandle);
    ApplyMovementLock(false);
    ActiveReactionState = EActionCombatReactionState::Knockdown;
    LastReactionImpulseDirection = WorldSpaceImpulseDirection;
    UpdateReactionTags();
    StartReactionCue(EActionCombatReactionState::Knockdown, WorldSpaceImpulseDirection);
    ApplyKnockdownLaunch(WorldSpaceImpulseDirection);
    GetWorld()->GetTimerManager().SetTimer(ReactionTimerHandle, this, &ThisClass::HandleReactionTimerExpired, FMath::Max(KnockdownDurationSeconds, 0.01f), false);
}

void UActionCombatReactionComponent::BeginGetUp()
{
    if (GetWorld() == nullptr)
    {
        return;
    }

    GetWorld()->GetTimerManager().ClearTimer(ReactionTimerHandle);
    ActiveReactionState = EActionCombatReactionState::GetUp;
    UpdateReactionTags();
    StartReactionCue(EActionCombatReactionState::GetUp, LastReactionImpulseDirection);
    ApplyMovementLock(bDisableMovementDuringHitReaction);
    GetWorld()->GetTimerManager().SetTimer(ReactionTimerHandle, this, &ThisClass::HandleReactionTimerExpired, FMath::Max(GetUpDurationSeconds, 0.01f), false);
}

void UActionCombatReactionComponent::FinishReaction(float AdditionalImmunitySeconds)
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ReactionTimerHandle);
    }

    ActiveReactionState = EActionCombatReactionState::None;
    ApplyMovementLock(false);
    UpdateReactionTags();
    ReactionImmunityEndWorldTimeSeconds = GetCurrentWorldTimeSeconds() + FMath::Max(AdditionalImmunitySeconds, 0.0f);

    if (GetCurrentPoise() < (GetMaxPoise() - KINDA_SMALL_NUMBER))
    {
        SetComponentTickEnabled(true);
    }
}

void UActionCombatReactionComponent::HandleReactionTimerExpired()
{
    if (ActiveReactionState == EActionCombatReactionState::Knockdown)
    {
        BeginGetUp();
        return;
    }

    const float AdditionalImmunitySeconds = (ActiveReactionState == EActionCombatReactionState::GetUp)
        ? PostGetUpReactionImmunitySeconds
        : PostReactionImmunitySeconds;

    FinishReaction(AdditionalImmunitySeconds);
}

bool UActionCombatReactionComponent::InterruptCombatAction() const
{
    if (!bInterruptCombatActionsOnReaction)
    {
        return false;
    }

    if (UActionCombatComponent* ActionCombatComponent = GetOwner() ? GetOwner()->FindComponentByClass<UActionCombatComponent>() : nullptr)
    {
        ActionCombatComponent->InterruptActiveAction();
        return true;
    }

    return false;
}

void UActionCombatReactionComponent::ApplyMovementLock(bool bLockMovement)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    UCharacterMovementComponent* CharacterMovement = Character ? Character->GetCharacterMovement() : nullptr;
    if (CharacterMovement == nullptr)
    {
        return;
    }

    if (bLockMovement)
    {
        if (!bMovementLocked)
        {
            SavedMovementMode = CharacterMovement->MovementMode;
            SavedCustomMovementMode = CharacterMovement->CustomMovementMode;
            CharacterMovement->DisableMovement();
            bMovementLocked = true;
        }

        return;
    }

    if (bMovementLocked)
    {
        const EMovementMode RestoredMovementMode = (SavedMovementMode == MOVE_None) ? MOVE_Walking : static_cast<EMovementMode>(SavedMovementMode.GetValue());
        CharacterMovement->SetMovementMode(RestoredMovementMode, SavedCustomMovementMode);
        bMovementLocked = false;
    }
}

void UActionCombatReactionComponent::ApplyKnockdownLaunch(const FVector& WorldSpaceImpulseDirection)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (Character == nullptr)
    {
        return;
    }

    FVector LaunchDirection = WorldSpaceImpulseDirection.GetSafeNormal2D();
    if (LaunchDirection.IsNearlyZero())
    {
        LaunchDirection = -Character->GetActorForwardVector().GetSafeNormal2D();
    }

    const FVector LaunchVelocity = (LaunchDirection * FMath::Max(KnockdownLaunchSpeed, 0.0f))
        + FVector(0.0f, 0.0f, FMath::Max(KnockdownUpwardLaunchSpeed, 0.0f));

    Character->LaunchCharacter(LaunchVelocity, true, true);
}

void UActionCombatReactionComponent::StartReactionCue(EActionCombatReactionState NewState, const FVector& WorldSpaceImpulseDirection)
{
    if (!bAutoPlayReactionAnimations)
    {
        return;
    }

    ReplicatedReactionCueState = NewState;
    ReplicatedReactionCueDirection = WorldSpaceImpulseDirection.GetSafeNormal2D();
    ++ReplicatedReactionCueId;
    LastPlayedReactionCueId = ReplicatedReactionCueId;
    PlayReactionAnimation(NewState, ReplicatedReactionCueDirection);
}

void UActionCombatReactionComponent::PlayReactionAnimation(EActionCombatReactionState ReactionState, const FVector& WorldSpaceImpulseDirection)
{
    if (!bAutoPlayReactionAnimations || ReactionSlotName.IsNone())
    {
        return;
    }

    USkeletalMeshComponent* MeshComponent = ResolveAnimationMesh();
    UAnimInstance* AnimInstance = MeshComponent ? MeshComponent->GetAnimInstance() : nullptr;
    if (AnimInstance == nullptr)
    {
        return;
    }

    if (bStopPreviousReactionAnimation && ActiveReactionMontage)
    {
        AnimInstance->Montage_Stop(0.03f, ActiveReactionMontage);
        ActiveReactionMontage = nullptr;
    }

    const EActionCombatReactionDirection Direction = ResolveReactionDirection(WorldSpaceImpulseDirection);
    const FActionCombatReactionAnimation* ReactionAnimation = FindReactionAnimation(ReactionState, Direction);
    if ((ReactionAnimation == nullptr) || !ReactionAnimation->HasAnimation())
    {
        return;
    }

    UAnimMontage* Montage = ReactionAnimation->Montage.LoadSynchronous();
    if (Montage)
    {
        if (AnimInstance->Montage_Play(Montage, FMath::Max(ReactionAnimation->PlayRate, 0.01f)) > 0.0f)
        {
            ActiveReactionMontage = Montage;
        }
        return;
    }

    UAnimSequenceBase* Sequence = ReactionAnimation->Sequence.LoadSynchronous();
    if (Sequence)
    {
        ActiveReactionMontage = AnimInstance->PlaySlotAnimationAsDynamicMontage(
            Sequence,
            ReactionSlotName,
            FMath::Max(ReactionAnimation->BlendInSeconds, 0.0f),
            FMath::Max(ReactionAnimation->BlendOutSeconds, 0.0f),
            FMath::Max(ReactionAnimation->PlayRate, 0.01f));
    }
}

const FActionCombatReactionAnimation* UActionCombatReactionComponent::FindReactionAnimation(EActionCombatReactionState ReactionState, EActionCombatReactionDirection Direction) const
{
    switch (ReactionState)
    {
    case EActionCombatReactionState::LightHit:
        return LightHitAnimations.FindAnimation(Direction);
    case EActionCombatReactionState::HeavyHit:
        return HeavyHitAnimations.FindAnimation(Direction);
    case EActionCombatReactionState::Knockdown:
        return KnockdownAnimation.HasAnimation() ? &KnockdownAnimation : HeavyHitAnimations.FindAnimation(Direction);
    case EActionCombatReactionState::GetUp:
        return GetUpAnimation.HasAnimation() ? &GetUpAnimation : nullptr;
    default:
        return nullptr;
    }
}

EActionCombatReactionDirection UActionCombatReactionComponent::ResolveReactionDirection(const FVector& WorldSpaceImpulseDirection) const
{
    const AActor* Owner = GetOwner();
    if (Owner == nullptr)
    {
        return EActionCombatReactionDirection::Front;
    }

    const FVector LocalDirection = Owner->GetActorTransform().InverseTransformVectorNoScale(WorldSpaceImpulseDirection.GetSafeNormal2D());
    if (FMath::Abs(LocalDirection.X) >= FMath::Abs(LocalDirection.Y))
    {
        return LocalDirection.X < 0.0f ? EActionCombatReactionDirection::Front : EActionCombatReactionDirection::Back;
    }

    return LocalDirection.Y < 0.0f ? EActionCombatReactionDirection::Right : EActionCombatReactionDirection::Left;
}

USkeletalMeshComponent* UActionCombatReactionComponent::ResolveAnimationMesh() const
{
    AActor* Owner = GetOwner();
    if (Owner == nullptr)
    {
        return nullptr;
    }

    TArray<USkeletalMeshComponent*> SkeletalMeshes;
    Owner->GetComponents(SkeletalMeshes);
    return SkeletalMeshes.Num() > 0 ? SkeletalMeshes[0] : nullptr;
}

void UActionCombatReactionComponent::OnRep_ReplicatedReactionCue()
{
    if (HasReactionAuthority() || LastPlayedReactionCueId == ReplicatedReactionCueId)
    {
        return;
    }

    LastPlayedReactionCueId = ReplicatedReactionCueId;
    PlayReactionAnimation(ReplicatedReactionCueState, ReplicatedReactionCueDirection);
}

double UActionCombatReactionComponent::GetCurrentWorldTimeSeconds() const
{
    return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}
