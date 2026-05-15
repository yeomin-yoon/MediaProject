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
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Engine/SkeletalMesh.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

namespace ActionCombatReactionComponentNames
{
    static const FName RuntimeComponentName(TEXT("ActionCombatReactionComponent_Runtime"));
}

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

    KnockdownAnimation.Sequence = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/1dev/OS/QuaterniusUAL2/Retargeted/Manny/Manny_Hit_Knockback.Manny_Hit_Knockback")));
    KnockdownAnimation.BlendInSeconds = 0.08f;
    KnockdownAnimation.BlendOutSeconds = 0.0f;

    GetUpAnimation.Sequence = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/1dev/OS/QuaterniusUAL2/Retargeted/Manny/Manny_LayToIdle.Manny_LayToIdle")));
    GetUpAnimation.BlendInSeconds = 0.02f;
    GetUpAnimation.BlendOutSeconds = 0.10f;
}

void UActionCombatReactionComponent::BeginPlay()
{
    Super::BeginPlay();
    EnsureReactionSet();
}

void UActionCombatReactionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    TickKnockdownActorDisplacement(DeltaTime);

    if (!HasReactionAuthority() || IsInReactionState())
    {
        return;
    }

    const double CurrentWorldTimeSeconds = GetCurrentWorldTimeSeconds();
    if ((LastIncomingHitWorldTimeSeconds >= 0.0) && ((CurrentWorldTimeSeconds - LastIncomingHitWorldTimeSeconds) < PoiseRecoveryDelaySeconds))
    {
        return;
    }

    EnsureReactionSet();
    if (LocalPoise < 0.0f)
    {
        LocalPoise = GetMaxPoise();
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
    DOREPLIFETIME(ThisClass, ReplicatedReactionCueLocation);
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

    if (UActionCombatReactionComponent* ExistingNamedComponent = FindObject<UActionCombatReactionComponent>(Actor, *ActionCombatReactionComponentNames::RuntimeComponentName.ToString()))
    {
        return ExistingNamedComponent;
    }

    UActionCombatReactionComponent* NewComponent = NewObject<UActionCombatReactionComponent>(Actor, ActionCombatReactionComponentNames::RuntimeComponentName);
    if (NewComponent == nullptr)
    {
        return nullptr;
    }

    NewComponent->SetNetAddressable();
    NewComponent->SetIsReplicated(true);
    Actor->AddInstanceComponent(NewComponent);
    NewComponent->RegisterComponent();
    return NewComponent;
}

void UActionCombatReactionComponent::PlayReactionCueOnActor(AActor* Actor, EActionCombatReactionState NewState, FVector_NetQuantizeNormal WorldSpaceImpulseDirection, FVector_NetQuantize WorldSpaceActorLocation, int32 CueId)
{
    if (Actor == nullptr)
    {
        return;
    }

    if (UActionCombatReactionComponent* ExistingComponent = FindReactionComponent(Actor))
    {
        ExistingComponent->PlayReplicatedReactionCue(NewState, WorldSpaceImpulseDirection, WorldSpaceActorLocation, CueId);
        return;
    }

    if (Actor->HasAuthority())
    {
        return;
    }

    UActionCombatReactionComponent* LocalCueComponent = FindObject<UActionCombatReactionComponent>(Actor, *ActionCombatReactionComponentNames::RuntimeComponentName.ToString());
    if (LocalCueComponent == nullptr)
    {
        LocalCueComponent = NewObject<UActionCombatReactionComponent>(Actor, ActionCombatReactionComponentNames::RuntimeComponentName);
        if (LocalCueComponent == nullptr)
        {
            return;
        }

        LocalCueComponent->SetNetAddressable();
        LocalCueComponent->SetIsReplicated(false);
        Actor->AddInstanceComponent(LocalCueComponent);
        LocalCueComponent->RegisterComponent();
    }

    LocalCueComponent->PlayReplicatedReactionCue(NewState, WorldSpaceImpulseDirection, WorldSpaceActorLocation, CueId);
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

    LastReactionInstigatorActor = IncomingHit.InstigatorActor;

    if (EnsureReactionSet() == nullptr)
    {
        if (LocalPoise < 0.0f)
        {
            LocalPoise = GetMaxPoise();
        }

        UE_LOG(
            LogActionCombatRuntime,
            Verbose,
            TEXT("[Reaction:%s] Using local fallback poise because no ActionCombatReactionSet/ASC was available."),
            *GetPathNameSafe(GetOwner()));
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
        TEXT("[Reaction:%s] Outcome=%d PoiseBefore=%.2f PoiseAfter=%.2f PoiseDamage=%.2f KnockdownPower=%.2f KnockdownThreshold=%.2f BreakChain=%d"),
        *GetPathNameSafe(GetOwner()),
        static_cast<int32>(OutResult.Outcome),
        OutResult.PoiseBefore,
        OutResult.PoiseAfter,
        EffectivePoiseDamage,
        EffectiveKnockdownPower,
        GetKnockdownThreshold(),
        OutResult.PoiseBreakChainCount);

    return OutResult.Outcome != EActionCombatReactionOutcome::None;
}

bool UActionCombatReactionComponent::ApplyReactionHit(AActor* InstigatorActor, float PoiseDamage, float KnockdownPower, FVector WorldSpaceImpulseDirection, FActionCombatReactionResult& OutResult)
{
    FActionCombatReactionHit ReactionHit;
    ReactionHit.PoiseDamage = PoiseDamage;
    ReactionHit.KnockdownPower = KnockdownPower;
    ReactionHit.WorldSpaceImpulseDirection = ResolveImpulseDirection(InstigatorActor, WorldSpaceImpulseDirection);
    ReactionHit.InstigatorActor = InstigatorActor;

    return TryApplyReactionHit(ReactionHit, OutResult);
}

bool UActionCombatReactionComponent::ApplyReactionHitToActor(AActor* TargetActor, AActor* InstigatorActor, float PoiseDamage, float KnockdownPower, FVector WorldSpaceImpulseDirection, FActionCombatReactionResult& OutResult)
{
    OutResult = FActionCombatReactionResult();

    if (UActionCombatReactionComponent* ReactionComponent = FindOrCreateReactionComponent(TargetActor))
    {
        return ReactionComponent->ApplyReactionHit(InstigatorActor, PoiseDamage, KnockdownPower, WorldSpaceImpulseDirection, OutResult);
    }

    return false;
}

bool UActionCombatReactionComponent::ForceKnockdown(AActor* InstigatorActor, FVector WorldSpaceImpulseDirection)
{
    return ForceKnockdownInternal(InstigatorActor, WorldSpaceImpulseDirection, nullptr);
}

bool UActionCombatReactionComponent::ForceKnockdownActor(AActor* TargetActor, AActor* InstigatorActor, FVector WorldSpaceImpulseDirection)
{
    if (UActionCombatReactionComponent* ReactionComponent = FindOrCreateReactionComponent(TargetActor))
    {
        return ReactionComponent->ForceKnockdown(InstigatorActor, WorldSpaceImpulseDirection);
    }

    return false;
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

    if (LocalPoise < 0.0f)
    {
        LocalPoise = GetMaxPoise();
    }

    return LocalPoise;
}

float UActionCombatReactionComponent::GetMaxPoise() const
{
    if (const UActionCombatReactionSet* ReactionSet = FindReactionSet())
    {
        return FMath::Max(ReactionSet->GetMaxPoise(), 1.0f);
    }

    return FMath::Max(FallbackMaxPoise, 1.0f);
}

float UActionCombatReactionComponent::GetPoiseRecoveryRate() const
{
    if (const UActionCombatReactionSet* ReactionSet = FindReactionSet())
    {
        return FMath::Max(ReactionSet->GetPoiseRecoveryRate(), 0.0f);
    }

    return FMath::Max(FallbackPoiseRecoveryRate, 0.0f);
}

float UActionCombatReactionComponent::GetKnockdownThreshold() const
{
    if (const UActionCombatReactionSet* ReactionSet = FindReactionSet())
    {
        return FMath::Max(ReactionSet->GetKnockdownThreshold(), 0.0f);
    }

    return FMath::Max(FallbackKnockdownThreshold, 0.0f);
}

void UActionCombatReactionComponent::SetCurrentPoise(float NewValue) const
{
    if (UAbilitySystemComponent* AbilitySystemComponent = ResolveAbilitySystemComponent())
    {
        AbilitySystemComponent->SetNumericAttributeBase(UActionCombatReactionSet::GetPoiseAttribute(), FMath::Clamp(NewValue, 0.0f, GetMaxPoise()));
        return;
    }

    LocalPoise = FMath::Clamp(NewValue, 0.0f, GetMaxPoise());
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
    ClearKnockdownPoseHold(true);
    ClearKnockdownActorDisplacement();
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
    ClearKnockdownPoseHold(true);
    ApplyMovementLock(bDisableMovementDuringHitReaction);
    ActiveReactionState = EActionCombatReactionState::Knockdown;
    LastReactionImpulseDirection = WorldSpaceImpulseDirection;
    ApplyReactionFacing(WorldSpaceImpulseDirection);
    UpdateReactionTags();
    StartReactionCue(EActionCombatReactionState::Knockdown, WorldSpaceImpulseDirection);
    ApplyKnockdownLaunch(WorldSpaceImpulseDirection);
    const float KnockdownAnimationSeconds = GetReactionAnimationPlayLengthSeconds(EActionCombatReactionState::Knockdown, WorldSpaceImpulseDirection, KnockdownDurationSeconds);
    const float GetUpStartDelaySeconds = FMath::Max(KnockdownAnimationSeconds - FMath::Max(GetUpHoldReleaseDelaySeconds, 0.0f), 0.01f);
    StartKnockdownActorDisplacement(WorldSpaceImpulseDirection, GetUpStartDelaySeconds);
    GetWorld()->GetTimerManager().SetTimer(ReactionTimerHandle, this, &ThisClass::HandleReactionTimerExpired, GetUpStartDelaySeconds, false);
}

void UActionCombatReactionComponent::BeginGetUp()
{
    if (GetWorld() == nullptr)
    {
        return;
    }

    GetWorld()->GetTimerManager().ClearTimer(ReactionTimerHandle);
    ClearKnockdownActorDisplacement();
    if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
        if (UCharacterMovementComponent* CharacterMovement = Character->GetCharacterMovement())
        {
            CharacterMovement->StopMovementImmediately();
        }
    }

    ActiveReactionState = EActionCombatReactionState::GetUp;
    ApplyReactionFacing(LastReactionImpulseDirection);
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

    ClearKnockdownPoseHold(true);
    ClearKnockdownActorDisplacement();
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

bool UActionCombatReactionComponent::ForceKnockdownInternal(AActor* InstigatorActor, const FVector& WorldSpaceImpulseDirection, FActionCombatReactionResult* OutResult)
{
    if (!HasReactionAuthority())
    {
        UE_LOG(
            LogActionCombatRuntime,
            Warning,
            TEXT("[Reaction:%s] ForceKnockdown ignored because the caller did not have authority."),
            *GetPathNameSafe(GetOwner()));
        return false;
    }

    const FVector ResolvedImpulseDirection = ResolveImpulseDirection(InstigatorActor, WorldSpaceImpulseDirection);
    const double CurrentWorldTimeSeconds = GetCurrentWorldTimeSeconds();

    if (OutResult)
    {
        OutResult->Outcome = EActionCombatReactionOutcome::Knockdown;
        OutResult->PoiseBefore = GetCurrentPoise();
        OutResult->PoiseAfter = GetMaxPoise();
        OutResult->PoiseBreakChainCount = 0;
    }

    const bool bInterruptedCombatAction = InterruptCombatAction();
    if (OutResult)
    {
        OutResult->bInterruptedCombatAction = bInterruptedCombatAction;
    }

    ReactionImmunityEndWorldTimeSeconds = -1.0;
    LastIncomingHitWorldTimeSeconds = CurrentWorldTimeSeconds;
    LastPoiseBreakWorldTimeSeconds = CurrentWorldTimeSeconds;
    RecentPoiseBreakCount = 0;
    SetCurrentPoise(GetMaxPoise());
    SetComponentTickEnabled(true);
    BeginKnockdown(ResolvedImpulseDirection);

    UE_LOG(
        LogActionCombatRuntime,
        Log,
        TEXT("[Reaction:%s] ForceKnockdown Instigator=%s Direction=%s"),
        *GetPathNameSafe(GetOwner()),
        *GetPathNameSafe(InstigatorActor),
        *ResolvedImpulseDirection.ToCompactString());

    return true;
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

void UActionCombatReactionComponent::ApplyReactionFacing(const FVector& WorldSpaceImpulseDirection)
{
    if (!bFaceReactionSourceOnKnockdown)
    {
        return;
    }

    AActor* Owner = GetOwner();
    if (Owner == nullptr)
    {
        return;
    }

    FVector FacingDirection = -WorldSpaceImpulseDirection.GetSafeNormal2D();
    if (FacingDirection.IsNearlyZero())
    {
        if (AActor* InstigatorActor = LastReactionInstigatorActor.Get())
        {
            FacingDirection = (InstigatorActor->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal2D();
        }
    }

    if (FacingDirection.IsNearlyZero())
    {
        return;
    }

    const FRotator DesiredRotation(0.0f, FacingDirection.Rotation().Yaw, 0.0f);
    if (ACharacter* Character = Cast<ACharacter>(Owner))
    {
        if (AController* Controller = Character->GetController())
        {
            FRotator ControlRotation = Controller->GetControlRotation();
            ControlRotation.Yaw = DesiredRotation.Yaw;
            Controller->SetControlRotation(ControlRotation);
        }

        Character->FaceRotation(DesiredRotation, 0.0f);
    }

    Owner->SetActorRotation(DesiredRotation, ETeleportType::TeleportPhysics);
    Owner->ForceNetUpdate();
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

    const FVector LaunchVelocity = FVector(0.0f, 0.0f, FMath::Max(KnockdownUpwardLaunchSpeed, 0.0f));

    if (!LaunchVelocity.IsNearlyZero())
    {
        Character->LaunchCharacter(LaunchVelocity, true, true);
    }
}

void UActionCombatReactionComponent::StartKnockdownActorDisplacement(const FVector& WorldSpaceImpulseDirection, float DurationSeconds)
{
    AActor* Owner = GetOwner();
    if ((Owner == nullptr) || (KnockdownActorDisplacementDistance <= 0.0f))
    {
        return;
    }

    FVector SlideDirection = WorldSpaceImpulseDirection.GetSafeNormal2D();
    if (SlideDirection.IsNearlyZero())
    {
        SlideDirection = -Owner->GetActorForwardVector().GetSafeNormal2D();
    }

    if (SlideDirection.IsNearlyZero())
    {
        return;
    }

    if (ACharacter* Character = Cast<ACharacter>(Owner))
    {
        if (UCharacterMovementComponent* CharacterMovement = Character->GetCharacterMovement())
        {
            CharacterMovement->StopMovementImmediately();
        }
    }

    KnockdownActorDisplacementDirection = SlideDirection;
    KnockdownActorDisplacementRemainingDistance = FMath::Max(KnockdownActorDisplacementDistance, 0.0f);
    KnockdownActorDisplacementSpeed = KnockdownActorDisplacementRemainingDistance / FMath::Max(DurationSeconds, 0.01f);
    SetComponentTickEnabled(true);
}

void UActionCombatReactionComponent::TickKnockdownActorDisplacement(float DeltaTime)
{
    if (KnockdownActorDisplacementRemainingDistance <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    AActor* Owner = GetOwner();
    if ((Owner == nullptr) || KnockdownActorDisplacementDirection.IsNearlyZero())
    {
        ClearKnockdownActorDisplacement();
        return;
    }

    if (ACharacter* Character = Cast<ACharacter>(Owner))
    {
        if (UCharacterMovementComponent* CharacterMovement = Character->GetCharacterMovement())
        {
            CharacterMovement->StopMovementImmediately();
        }
    }

    const float RequestedDistance = FMath::Min(KnockdownActorDisplacementRemainingDistance, KnockdownActorDisplacementSpeed * FMath::Max(DeltaTime, 0.0f));
    const FVector PreviousLocation = Owner->GetActorLocation();
    FHitResult MoveHit;
    Owner->AddActorWorldOffset(KnockdownActorDisplacementDirection * RequestedDistance, true, &MoveHit, ETeleportType::None);

    const float ActualDistance = FMath::Max(FVector::DotProduct(Owner->GetActorLocation() - PreviousLocation, KnockdownActorDisplacementDirection), 0.0f);
    KnockdownActorDisplacementRemainingDistance -= ActualDistance;

    if ((ActualDistance <= KINDA_SMALL_NUMBER) || MoveHit.IsValidBlockingHit())
    {
        ClearKnockdownActorDisplacement();
    }

    Owner->ForceNetUpdate();
}

void UActionCombatReactionComponent::ClearKnockdownActorDisplacement()
{
    KnockdownActorDisplacementDirection = FVector::ZeroVector;
    KnockdownActorDisplacementRemainingDistance = 0.0f;
    KnockdownActorDisplacementSpeed = 0.0f;
}

void UActionCombatReactionComponent::StartReactionCue(EActionCombatReactionState NewState, const FVector& WorldSpaceImpulseDirection)
{
    if (!bAutoPlayReactionAnimations)
    {
        return;
    }

    ReplicatedReactionCueState = NewState;
    ReplicatedReactionCueDirection = WorldSpaceImpulseDirection.GetSafeNormal2D();
    ReplicatedReactionCueLocation = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
    ++ReplicatedReactionCueId;
    LastPlayedReactionCueId = ReplicatedReactionCueId;
    PlayReactionAnimation(NewState, ReplicatedReactionCueDirection);

    if (AActor* Owner = GetOwner())
    {
        Owner->ForceNetUpdate();

        bool bRelayedThroughExistingCombatComponent = false;
        if (UActionCombatComponent* OwnerCombatComponent = Owner->FindComponentByClass<UActionCombatComponent>())
        {
            OwnerCombatComponent->BroadcastReactionCueForActor(Owner, NewState, ReplicatedReactionCueDirection, ReplicatedReactionCueLocation, ReplicatedReactionCueId);
            bRelayedThroughExistingCombatComponent = true;
        }

        if (!bRelayedThroughExistingCombatComponent)
        {
            if (AActor* InstigatorActor = LastReactionInstigatorActor.Get())
            {
                if (UActionCombatComponent* InstigatorCombatComponent = InstigatorActor->FindComponentByClass<UActionCombatComponent>())
                {
                    InstigatorCombatComponent->BroadcastReactionCueForActor(Owner, NewState, ReplicatedReactionCueDirection, ReplicatedReactionCueLocation, ReplicatedReactionCueId);
                }
            }
        }
    }

    MulticastPlayReactionCue(NewState, ReplicatedReactionCueDirection, ReplicatedReactionCueLocation, ReplicatedReactionCueId);
}

void UActionCombatReactionComponent::PlayReactionAnimation(EActionCombatReactionState ReactionState, const FVector& WorldSpaceImpulseDirection)
{
    if (!bAutoPlayReactionAnimations || ReactionSlotName.IsNone())
    {
        return;
    }

    const EActionCombatReactionDirection Direction = ResolveReactionDirection(WorldSpaceImpulseDirection);
    const FActionCombatReactionAnimation* ReactionAnimation = FindReactionAnimation(ReactionState, Direction);
    if ((ReactionAnimation == nullptr) || !ReactionAnimation->HasAnimation())
    {
        UE_LOG(
            LogActionCombatRuntime,
            Warning,
            TEXT("[Reaction:%s] No reaction animation assigned for State=%d Direction=%d"),
            *GetPathNameSafe(GetOwner()),
            static_cast<int32>(ReactionState),
            static_cast<int32>(Direction));
        return;
    }

    UAnimMontage* SourceMontage = ReactionAnimation->Montage.LoadSynchronous();
    UAnimMontage* PlayableMontage = SourceMontage ? ResolvePlayableReactionMontage(SourceMontage) : nullptr;
    UAnimSequenceBase* Sequence = SourceMontage ? nullptr : ReactionAnimation->Sequence.LoadSynchronous();

    USkeletalMeshComponent* MeshComponent = ResolveAnimationMesh(ReactionAnimation);
    UAnimInstance* AnimInstance = MeshComponent ? MeshComponent->GetAnimInstance() : nullptr;
    if (AnimInstance == nullptr)
    {
        UE_LOG(
            LogActionCombatRuntime,
            Warning,
            TEXT("[Reaction:%s] Could not play reaction animation. Mesh=%s AnimInstance=%s State=%d"),
            *GetPathNameSafe(GetOwner()),
            *GetPathNameSafe(MeshComponent),
            *GetPathNameSafe(AnimInstance),
            static_cast<int32>(ReactionState));
        return;
    }

    const bool bTransitioningFromHeldKnockdownToGetUp =
        (ReactionState == EActionCombatReactionState::GetUp) && (HeldKnockdownMontage != nullptr);

    if (bStopPreviousReactionAnimation && !bTransitioningFromHeldKnockdownToGetUp)
    {
        AnimInstance->Montage_Stop(0.03f);
        ActiveReactionMontage = nullptr;
        ClearKnockdownPoseHold(false);
    }

    if (PlayableMontage)
    {
        if (AnimInstance->Montage_Play(PlayableMontage, FMath::Max(ReactionAnimation->PlayRate, 0.01f)) > 0.0f)
        {
            ActiveReactionMontage = PlayableMontage;
            ActiveReactionAnimInstance = AnimInstance;
            if (ReactionState == EActionCombatReactionState::Knockdown)
            {
                ScheduleKnockdownPoseHold(AnimInstance, ActiveReactionMontage, ReactionAnimation->PlayRate);
            }
            else if (ReactionState == EActionCombatReactionState::GetUp)
            {
                if (HeldKnockdownMontage && GetWorld())
                {
                    GetWorld()->GetTimerManager().SetTimer(KnockdownPoseHoldClearTimerHandle, this, &ThisClass::ClearKnockdownPoseHoldDeferred, FMath::Max(GetUpHoldReleaseDelaySeconds, 0.0f), false);
                }
                else
                {
                    ClearKnockdownPoseHold(true);
                }
            }
            UE_LOG(
                LogActionCombatRuntime,
                Log,
                TEXT("[Reaction:%s] Playing reaction montage Source=%s Playable=%s on %s Slot=%s State=%d"),
                *GetPathNameSafe(GetOwner()),
                *GetPathNameSafe(SourceMontage),
                *GetPathNameSafe(PlayableMontage),
                *GetPathNameSafe(MeshComponent),
                *ReactionSlotName.ToString(),
                static_cast<int32>(ReactionState));
        }
        else
        {
            UE_LOG(
                LogActionCombatRuntime,
                Warning,
                TEXT("[Reaction:%s] Montage_Play failed for %s on %s State=%d"),
                *GetPathNameSafe(GetOwner()),
                *GetPathNameSafe(PlayableMontage),
                *GetPathNameSafe(MeshComponent),
                static_cast<int32>(ReactionState));
        }
        return;
    }

    if (Sequence)
    {
        ActiveReactionMontage = AnimInstance->PlaySlotAnimationAsDynamicMontage(
            Sequence,
            ReactionSlotName,
            FMath::Max(ReactionAnimation->BlendInSeconds, 0.0f),
            FMath::Max(ReactionAnimation->BlendOutSeconds, 0.0f),
            FMath::Max(ReactionAnimation->PlayRate, 0.01f));

        if (ActiveReactionMontage)
        {
            ActiveReactionAnimInstance = AnimInstance;
            if (ReactionState == EActionCombatReactionState::Knockdown)
            {
                ScheduleKnockdownPoseHold(AnimInstance, ActiveReactionMontage, ReactionAnimation->PlayRate);
            }
            else if (ReactionState == EActionCombatReactionState::GetUp)
            {
                if (HeldKnockdownMontage && GetWorld())
                {
                    GetWorld()->GetTimerManager().SetTimer(KnockdownPoseHoldClearTimerHandle, this, &ThisClass::ClearKnockdownPoseHoldDeferred, FMath::Max(GetUpHoldReleaseDelaySeconds, 0.0f), false);
                }
                else
                {
                    ClearKnockdownPoseHold(true);
                }
            }
            UE_LOG(
                LogActionCombatRuntime,
                Log,
                TEXT("[Reaction:%s] Playing reaction sequence %s as dynamic montage on %s Slot=%s State=%d"),
                *GetPathNameSafe(GetOwner()),
                *GetPathNameSafe(Sequence),
                *GetPathNameSafe(MeshComponent),
                *ReactionSlotName.ToString(),
                static_cast<int32>(ReactionState));
        }
        else
        {
            UE_LOG(
                LogActionCombatRuntime,
                Warning,
                TEXT("[Reaction:%s] PlaySlotAnimationAsDynamicMontage failed for %s on %s Slot=%s State=%d"),
                *GetPathNameSafe(GetOwner()),
                *GetPathNameSafe(Sequence),
                *GetPathNameSafe(MeshComponent),
                *ReactionSlotName.ToString(),
                static_cast<int32>(ReactionState));
        }
    }
}

void UActionCombatReactionComponent::ClearKnockdownPoseHold(bool bStopHeldMontage)
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(KnockdownPoseHoldTimerHandle);
        GetWorld()->GetTimerManager().ClearTimer(KnockdownPoseHoldClearTimerHandle);
    }

    UAnimMontage* MontageToClear = HeldKnockdownMontage;
    HeldKnockdownMontage = nullptr;

    if (bStopHeldMontage && MontageToClear)
    {
        if (UAnimInstance* AnimInstance = ActiveReactionAnimInstance.Get())
        {
            AnimInstance->Montage_Stop(0.0f, MontageToClear);
        }
    }

    if (ActiveReactionMontage == MontageToClear)
    {
        ActiveReactionMontage = nullptr;
    }
}

void UActionCombatReactionComponent::ClearKnockdownPoseHoldDeferred()
{
    ClearKnockdownPoseHold(true);
}

void UActionCombatReactionComponent::ScheduleKnockdownPoseHold(UAnimInstance* AnimInstance, UAnimMontage* KnockdownMontage, float PlayRate)
{
    if (!bHoldKnockdownPoseUntilGetUp || (GetWorld() == nullptr) || (AnimInstance == nullptr) || (KnockdownMontage == nullptr))
    {
        return;
    }

    GetWorld()->GetTimerManager().ClearTimer(KnockdownPoseHoldTimerHandle);
    HeldKnockdownMontage = KnockdownMontage;
    ActiveReactionAnimInstance = AnimInstance;

    const float SafePlayRate = FMath::Max(PlayRate, 0.01f);
    const float HoldDelaySeconds = FMath::Max((KnockdownMontage->GetPlayLength() / SafePlayRate) - FMath::Max(KnockdownPoseHoldLeadSeconds, 0.0f), 0.01f);
    GetWorld()->GetTimerManager().SetTimer(KnockdownPoseHoldTimerHandle, this, &ThisClass::HoldKnockdownPose, HoldDelaySeconds, false);
}

void UActionCombatReactionComponent::HoldKnockdownPose()
{
    UAnimInstance* AnimInstance = ActiveReactionAnimInstance.Get();
    UAnimMontage* KnockdownMontage = HeldKnockdownMontage;
    if ((AnimInstance == nullptr) || (KnockdownMontage == nullptr))
    {
        return;
    }

    const float HoldPosition = FMath::Max(KnockdownMontage->GetPlayLength() - FMath::Max(KnockdownPoseHoldLeadSeconds, 0.0f), 0.0f);
    AnimInstance->Montage_SetPosition(KnockdownMontage, HoldPosition);
    AnimInstance->Montage_Pause(KnockdownMontage);
}

void UActionCombatReactionComponent::ApplyReplicatedReactionLocation(EActionCombatReactionState NewState, const FVector& WorldSpaceActorLocation)
{
    if (!bSnapClientToServerReactionLocation || HasReactionAuthority())
    {
        return;
    }

    AActor* Owner = GetOwner();
    if ((Owner == nullptr) || WorldSpaceActorLocation.ContainsNaN())
    {
        return;
    }

    if (NewState != EActionCombatReactionState::GetUp)
    {
        return;
    }

    const float MaxSnapDistance = 500.0f;
    const float DistanceToServerLocation = FVector::Dist2D(Owner->GetActorLocation(), WorldSpaceActorLocation);
    if (DistanceToServerLocation <= KINDA_SMALL_NUMBER || DistanceToServerLocation > MaxSnapDistance)
    {
        return;
    }

    Owner->SetActorLocation(WorldSpaceActorLocation, false, nullptr, ETeleportType::TeleportPhysics);
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

bool UActionCombatReactionComponent::ShouldUseReactionSlotOverride(const UAnimMontage* SourceMontage) const
{
    return SourceMontage
        && !ReactionSlotName.IsNone()
        && SourceMontage->SlotAnimTracks.Num() == 1
        && SourceMontage->SlotAnimTracks[0].SlotName != ReactionSlotName;
}

UAnimMontage* UActionCombatReactionComponent::ResolvePlayableReactionMontage(UAnimMontage* SourceMontage)
{
    if (!ShouldUseReactionSlotOverride(SourceMontage))
    {
        return SourceMontage;
    }

    if (TObjectPtr<UAnimMontage>* ExistingOverride = RuntimeReactionMontageOverrides.Find(SourceMontage))
    {
        return ExistingOverride->Get();
    }

    UAnimMontage* OverrideMontage = DuplicateObject<UAnimMontage>(SourceMontage, this);
    if (!OverrideMontage)
    {
        return SourceMontage;
    }

    OverrideMontage->SlotAnimTracks[0].SlotName = ReactionSlotName;
    RuntimeReactionMontageOverrides.Add(SourceMontage, OverrideMontage);
    return OverrideMontage;
}

bool UActionCombatReactionComponent::IsAnimationCompatibleWithMesh(const USkeletalMeshComponent* MeshComponent, const FActionCombatReactionAnimation* ReactionAnimation) const
{
    const USkeletalMesh* SkeletalMesh = MeshComponent ? MeshComponent->GetSkeletalMeshAsset() : nullptr;
    const USkeleton* MeshSkeleton = SkeletalMesh ? SkeletalMesh->GetSkeleton() : nullptr;
    if ((MeshSkeleton == nullptr) || (ReactionAnimation == nullptr))
    {
        return false;
    }

    if (const UAnimMontage* Montage = ReactionAnimation->Montage.LoadSynchronous())
    {
        return Montage->GetSkeleton() == MeshSkeleton;
    }

    if (const UAnimSequenceBase* Sequence = ReactionAnimation->Sequence.LoadSynchronous())
    {
        return Sequence->GetSkeleton() == MeshSkeleton;
    }

    return false;
}

float UActionCombatReactionComponent::GetReactionAnimationPlayLengthSeconds(EActionCombatReactionState ReactionState, const FVector& WorldSpaceImpulseDirection, float FallbackSeconds) const
{
    const EActionCombatReactionDirection Direction = ResolveReactionDirection(WorldSpaceImpulseDirection);
    const FActionCombatReactionAnimation* ReactionAnimation = FindReactionAnimation(ReactionState, Direction);
    if ((ReactionAnimation == nullptr) || !ReactionAnimation->HasAnimation())
    {
        return FMath::Max(FallbackSeconds, 0.01f);
    }

    const float SafePlayRate = FMath::Max(ReactionAnimation->PlayRate, 0.01f);
    if (const UAnimMontage* Montage = ReactionAnimation->Montage.LoadSynchronous())
    {
        return FMath::Max(Montage->GetPlayLength() / SafePlayRate, 0.01f);
    }

    if (const UAnimSequenceBase* Sequence = ReactionAnimation->Sequence.LoadSynchronous())
    {
        return FMath::Max(Sequence->GetPlayLength() / SafePlayRate, 0.01f);
    }

    return FMath::Max(FallbackSeconds, 0.01f);
}

USkeletalMeshComponent* UActionCombatReactionComponent::ResolveAnimationMesh(const FActionCombatReactionAnimation* ReactionAnimation) const
{
    AActor* Owner = GetOwner();
    if (Owner == nullptr)
    {
        return nullptr;
    }

    if (USkeletalMeshComponent* ExplicitMesh = Cast<USkeletalMeshComponent>(AnimationMeshComponent.GetComponent(Owner)))
    {
        if (ExplicitMesh->GetAnimInstance() && IsAnimationCompatibleWithMesh(ExplicitMesh, ReactionAnimation))
        {
            return ExplicitMesh;
        }
    }

    if (const UActionCombatComponent* CombatComponent = Owner->FindComponentByClass<UActionCombatComponent>())
    {
        if (USkeletalMeshComponent* CombatMesh = CombatComponent->GetAnimationMeshComponent())
        {
            if (CombatMesh->GetAnimInstance() && IsAnimationCompatibleWithMesh(CombatMesh, ReactionAnimation))
            {
                return CombatMesh;
            }
        }
    }

    TArray<USkeletalMeshComponent*> SkeletalMeshes;
    Owner->GetComponents(SkeletalMeshes);

    if (const ACharacter* Character = Cast<ACharacter>(Owner))
    {
        if (USkeletalMeshComponent* CharacterMesh = Character->GetMesh())
        {
            if (CharacterMesh->GetAnimInstance() && IsAnimationCompatibleWithMesh(CharacterMesh, ReactionAnimation))
            {
                return CharacterMesh;
            }
        }
    }

    USkeletalMeshComponent* FirstCompatibleMesh = nullptr;
    USkeletalMeshComponent* FirstAnimatedMesh = nullptr;
    for (USkeletalMeshComponent* MeshComponent : SkeletalMeshes)
    {
        if (MeshComponent == nullptr)
        {
            continue;
        }

        if ((FirstAnimatedMesh == nullptr) && MeshComponent->GetAnimInstance())
        {
            FirstAnimatedMesh = MeshComponent;
        }

        if (IsAnimationCompatibleWithMesh(MeshComponent, ReactionAnimation))
        {
            if (MeshComponent->GetAnimInstance())
            {
                return MeshComponent;
            }

            if (FirstCompatibleMesh == nullptr)
            {
                FirstCompatibleMesh = MeshComponent;
            }
        }
    }

    if (FirstCompatibleMesh)
    {
        return FirstCompatibleMesh;
    }

    if (FirstAnimatedMesh)
    {
        UE_LOG(
            LogActionCombatRuntime,
            Warning,
            TEXT("[Reaction:%s] No skeleton-compatible reaction mesh found. Falling back to animated mesh %s."),
            *GetPathNameSafe(GetOwner()),
            *GetPathNameSafe(FirstAnimatedMesh));
        return FirstAnimatedMesh;
    }

    return SkeletalMeshes.Num() > 0 ? SkeletalMeshes[0] : nullptr;
}

void UActionCombatReactionComponent::OnRep_ReplicatedReactionCue()
{
    if (HasReactionAuthority())
    {
        return;
    }

    PlayReplicatedReactionCue(ReplicatedReactionCueState, ReplicatedReactionCueDirection, ReplicatedReactionCueLocation, ReplicatedReactionCueId);
}

void UActionCombatReactionComponent::MulticastPlayReactionCue_Implementation(EActionCombatReactionState NewState, FVector_NetQuantizeNormal WorldSpaceImpulseDirection, FVector_NetQuantize WorldSpaceActorLocation, int32 CueId)
{
    PlayReplicatedReactionCue(NewState, WorldSpaceImpulseDirection, WorldSpaceActorLocation, CueId);
}

void UActionCombatReactionComponent::PlayReplicatedReactionCue(EActionCombatReactionState NewState, FVector_NetQuantizeNormal WorldSpaceImpulseDirection, FVector_NetQuantize WorldSpaceActorLocation, int32 CueId)
{
    ApplyReplicatedReactionLocation(NewState, WorldSpaceActorLocation);

    if (LastPlayedReactionCueId == CueId)
    {
        return;
    }

    LastPlayedReactionCueId = CueId;
    if ((NewState == EActionCombatReactionState::Knockdown) || (NewState == EActionCombatReactionState::GetUp))
    {
        ApplyReactionFacing(WorldSpaceImpulseDirection);
    }

    if ((NewState == EActionCombatReactionState::Knockdown) && !HasReactionAuthority())
    {
        const float KnockdownAnimationSeconds = GetReactionAnimationPlayLengthSeconds(EActionCombatReactionState::Knockdown, WorldSpaceImpulseDirection, KnockdownActorDisplacementDurationSeconds);
        StartKnockdownActorDisplacement(WorldSpaceImpulseDirection, FMath::Max(KnockdownAnimationSeconds - FMath::Max(GetUpHoldReleaseDelaySeconds, 0.0f), 0.01f));
    }
    PlayReactionAnimation(NewState, WorldSpaceImpulseDirection);
}

FVector UActionCombatReactionComponent::ResolveImpulseDirection(AActor* InstigatorActor, const FVector& WorldSpaceImpulseDirection) const
{
    FVector ResolvedImpulseDirection = WorldSpaceImpulseDirection.GetSafeNormal2D();
    if (!ResolvedImpulseDirection.IsNearlyZero())
    {
        return ResolvedImpulseDirection;
    }

    const AActor* Owner = GetOwner();
    if ((Owner != nullptr) && (InstigatorActor != nullptr))
    {
        ResolvedImpulseDirection = (Owner->GetActorLocation() - InstigatorActor->GetActorLocation()).GetSafeNormal2D();
        if (!ResolvedImpulseDirection.IsNearlyZero())
        {
            return ResolvedImpulseDirection;
        }
    }

    return Owner ? -Owner->GetActorForwardVector().GetSafeNormal2D() : FVector::BackwardVector;
}

double UActionCombatReactionComponent::GetCurrentWorldTimeSeconds() const
{
    return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}
