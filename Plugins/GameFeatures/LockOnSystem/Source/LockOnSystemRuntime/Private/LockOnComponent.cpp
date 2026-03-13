#include "LockOnComponent.h"

#include "DrawDebugHelpers.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "HAL/PlatformTime.h"
#include "LockOnSettings.h"
#include "LockOnSystemRuntimeLog.h"
#include "LockOnTargetComponent.h"
#include "LockOnWorldSubsystem.h"
#include "Math/RotationMatrix.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LockOnComponent)

namespace LockOnComponentPrivate
{
	static bool IsHorizontalCycleDirection(ELockOnCycleDirection CycleDirection)
	{
		return (CycleDirection == ELockOnCycleDirection::Left) || (CycleDirection == ELockOnCycleDirection::Right);
	}

	static bool IsPositiveCycleDirection(ELockOnCycleDirection CycleDirection)
	{
		return (CycleDirection == ELockOnCycleDirection::Right) || (CycleDirection == ELockOnCycleDirection::Up);
	}

	static float CalculateCameraSpaceAngleRadians(const FVector& ViewForward, const FVector& SignedAxis, const FVector& DirectionToTarget)
	{
		return FMath::Atan2(
			FVector::DotProduct(SignedAxis, DirectionToTarget),
			FVector::DotProduct(ViewForward, DirectionToTarget));
	}

	static FString BreakReasonToString(ELockOnBreakReason BreakReason)
	{
		return StaticEnum<ELockOnBreakReason>()->GetNameStringByValue((int64)BreakReason);
	}

	static FString CycleDirectionToString(ELockOnCycleDirection CycleDirection)
	{
		return StaticEnum<ELockOnCycleDirection>()->GetNameStringByValue((int64)CycleDirection);
	}

	static bool CompareCandidatesDeterministically(const FLockOnCandidate& Left, const FLockOnCandidate& Right)
	{
		if (!FMath::IsNearlyEqual(Left.ScoreBreakdown.TotalScore, Right.ScoreBreakdown.TotalScore))
		{
			return Left.ScoreBreakdown.TotalScore > Right.ScoreBreakdown.TotalScore;
		}

		const AActor* LeftActor = Left.TargetComponent.IsValid() ? Left.TargetComponent->GetOwner() : nullptr;
		const AActor* RightActor = Right.TargetComponent.IsValid() ? Right.TargetComponent->GetOwner() : nullptr;
		return GetNameSafe(LeftActor) < GetNameSafe(RightActor);
	}
}

ULockOnComponent::ULockOnComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);
}

void ULockOnComponent::BeginPlay()
{
	Super::BeginPlay();
	bDebugDrawEnabled = ULockOnSettings::Get()->bEnableDebugDrawByDefault;
}

void ULockOnComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearLocalHighlight();
	Super::EndPlay(EndPlayReason);
}

void ULockOnComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ShouldRunLocalPreview())
	{
		UpdateLocalPreview(DeltaTime);
		UpdateLocalHighlight();

		if (bDebugDrawEnabled)
		{
			DrawDebugState();
		}
	}
	else
	{
		LocalPreviewTargetActor.Reset();
		ClearLocalHighlight();
	}

	if (ShouldRunServerAuthority())
	{
		MaintainCheckAccumulator += DeltaTime;
		MaintainVisibilityAccumulator += DeltaTime;

		if (MaintainCheckAccumulator >= ULockOnSettings::Get()->MaintainCheckIntervalSeconds)
		{
			MaintainCheckAccumulator = 0.0f;
			MaintainCurrentLockServer();
		}
	}
}

void ULockOnComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ThisClass, ReplicatedState, COND_OwnerOnly);
}

void ULockOnComponent::RequestToggleLock()
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		ServerRequestToggleLock_Implementation();
	}
	else
	{
		ServerRequestToggleLock();
	}
}

void ULockOnComponent::RequestCycleLeft()
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		ServerRequestCycle_Implementation(ELockOnCycleDirection::Left);
	}
	else
	{
		ServerRequestCycle(ELockOnCycleDirection::Left);
	}
}

void ULockOnComponent::RequestCycleRight()
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		ServerRequestCycle_Implementation(ELockOnCycleDirection::Right);
	}
	else
	{
		ServerRequestCycle(ELockOnCycleDirection::Right);
	}
}

void ULockOnComponent::RequestCycleUp()
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		ServerRequestCycle_Implementation(ELockOnCycleDirection::Up);
	}
	else
	{
		ServerRequestCycle(ELockOnCycleDirection::Up);
	}
}

void ULockOnComponent::RequestCycleDown()
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		ServerRequestCycle_Implementation(ELockOnCycleDirection::Down);
	}
	else
	{
		ServerRequestCycle(ELockOnCycleDirection::Down);
	}
}

void ULockOnComponent::RequestClearLock()
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		ServerRequestClearLock_Implementation();
	}
	else
	{
		ServerRequestClearLock();
	}
}

AActor* ULockOnComponent::GetPreviewTargetActor() const
{
	return LocalPreviewTargetActor.Get();
}

bool ULockOnComponent::GetCurrentTargetFocusLocation(FVector& OutFocusLocation) const
{
	if (const ULockOnTargetComponent* TargetComponent = GetCurrentTargetComponent())
	{
		OutFocusLocation = TargetComponent->GetLockOnFocusLocation();
		return true;
	}

	if (ReplicatedState.TargetActor)
	{
		OutFocusLocation = ReplicatedState.TargetActor->GetActorLocation();
		return true;
	}

	OutFocusLocation = FVector::ZeroVector;
	return false;
}

void ULockOnComponent::SetDebugDrawEnabled(bool bEnabled)
{
	bDebugDrawEnabled = bEnabled;
}

void ULockOnComponent::DumpDebugState() const
{
	UE_LOG(LogLockOnSystem, Log,
		TEXT("LockOn State Locked=%s Target=%s Sequence=%u Preview=%s Queries=%d Traces=%d MaintainChecks=%d PreviewMismatch=%d RejectedCycles=%d DelegateRecoveries=%d"),
		IsLockActive() ? TEXT("true") : TEXT("false"),
		*GetNameSafe(ReplicatedState.TargetActor),
		ReplicatedState.Sequence,
		*GetNameSafe(LocalPreviewTargetActor.Get()),
		QueryCount,
		VisibilityTraceCount,
		MaintainCheckCount,
		PreviewMismatchCount,
		RejectedCycleRequestCount,
		DelegateChainRecoveryCount);

	const int32 MaxCandidatesToLog = FMath::Min(LastScoredCandidates.Num(), ULockOnSettings::Get()->MaxDebugCandidatesToDraw);
	for (int32 Index = 0; Index < MaxCandidatesToLog; ++Index)
	{
		const FLockOnCandidate& Candidate = LastScoredCandidates[Index];
		const AActor* TargetActor = Candidate.TargetComponent.IsValid() ? Candidate.TargetComponent->GetOwner() : nullptr;
		UE_LOG(LogLockOnSystem, Log,
			TEXT("Candidate[%d] %s Dist=%.1f Total=%.3f Screen=%.3f Distance=%.3f Visibility=%.3f Sticky=%.3f Priority=%.3f Visible=%s"),
			Index,
			*GetNameSafe(TargetActor),
			Candidate.Distance,
			Candidate.ScoreBreakdown.TotalScore,
			Candidate.ScoreBreakdown.ScreenCenterScore,
			Candidate.ScoreBreakdown.DistanceScore,
			Candidate.ScoreBreakdown.VisibilityScore,
			Candidate.ScoreBreakdown.StickyBonus,
			Candidate.ScoreBreakdown.TargetPriorityBonus,
			Candidate.bVisibilityConfirmed ? TEXT("true") : TEXT("false"));
	}
}

void ULockOnComponent::RunBenchmark(int32 Iterations)
{
	FVector ViewLocation;
	FRotator ViewRotation;
	if (!GetViewPoint(ViewLocation, ViewRotation))
	{
		UE_LOG(LogLockOnSystem, Warning, TEXT("LockOn benchmark failed: no valid viewpoint."));
		return;
	}

	const int32 SafeIterations = FMath::Max(1, Iterations);
	double FullScanCycles = 0.0;
	double SpatialCycles = 0.0;

	for (int32 Index = 0; Index < SafeIterations; ++Index)
	{
		TArray<FLockOnCandidate> Candidates;
		int32 TotalRegistered = 0;

		uint64 StartCycles = FPlatformTime::Cycles64();
		QueryAndScoreCandidates(ViewLocation, ViewRotation, false, false, false, false, Candidates, TotalRegistered);
		FullScanCycles += (double)(FPlatformTime::Cycles64() - StartCycles);

		StartCycles = FPlatformTime::Cycles64();
		QueryAndScoreCandidates(ViewLocation, ViewRotation, false, false, false, true, Candidates, TotalRegistered);
		SpatialCycles += (double)(FPlatformTime::Cycles64() - StartCycles);
	}

	const double MicrosPerCycle = FPlatformTime::GetSecondsPerCycle64() * 1000000.0;
	UE_LOG(LogLockOnSystem, Log,
		TEXT("LockOn benchmark Iterations=%d FullScanAvg=%.2fus SpatialAvg=%.2fus RegisteredTargets=%d"),
		SafeIterations,
		(FullScanCycles / SafeIterations) * MicrosPerCycle,
		(SpatialCycles / SafeIterations) * MicrosPerCycle,
		GetLockOnWorldSubsystem() ? GetLockOnWorldSubsystem()->GetRegisteredTargetCount() : 0);
}

void ULockOnComponent::NotifyDelegateChainRecovered()
{
	++DelegateChainRecoveryCount;
}

void ULockOnComponent::ServerRequestToggleLock_Implementation()
{
	if (IsLockActive())
	{
		ClearLockServer(ELockOnBreakReason::ExplicitClear);
	}
	else
	{
		AcquireBestTargetServer();
	}
}

void ULockOnComponent::ServerRequestCycle_Implementation(ELockOnCycleDirection CycleDirection)
{
	if (!CanProcessServerCycleRequest(CycleDirection))
	{
		return;
	}

	CycleTargetServer(CycleDirection);
}

void ULockOnComponent::ServerRequestClearLock_Implementation()
{
	ClearLockServer(ELockOnBreakReason::ExplicitClear);
}

void ULockOnComponent::OnRep_ReplicatedState()
{
	HandleLockStateChanged();
}

bool ULockOnComponent::AcquireBestTargetServer()
{
	FVector ViewLocation;
	FRotator ViewRotation;
	if (!GetViewPoint(ViewLocation, ViewRotation))
	{
		return false;
	}

	TArray<FLockOnCandidate> Candidates;
	int32 TotalRegistered = 0;
	const bool bAllowVisibilityTrace = ULockOnSettings::Get()->VisibilityTracePolicy != ELockOnVisibilityTracePolicy::Disabled;
	QueryAndScoreCandidates(ViewLocation, ViewRotation, false, bAllowVisibilityTrace, true, true, Candidates, TotalRegistered);
	RefreshDebugCandidatesFromLastQuery(Candidates);

	FLockOnCandidate BestCandidate;
	if (!ChooseBestCandidate(Candidates, BestCandidate) || !BestCandidate.TargetComponent.IsValid())
	{
		ClearLockServer(ELockOnBreakReason::InvalidTarget);
		return false;
	}

	CommitLockState(BestCandidate.TargetComponent->GetOwner(), ELockOnBreakReason::None);
	SecondsSinceVisibilityLoss = -1.0f;
	return true;
}

bool ULockOnComponent::CycleTargetServer(ELockOnCycleDirection CycleDirection)
{
	if (!IsLockActive())
	{
		return AcquireBestTargetServer();
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	if (!GetViewPoint(ViewLocation, ViewRotation))
	{
		return false;
	}

	TArray<FLockOnCandidate> Candidates;
	int32 TotalRegistered = 0;
	const bool bAllowVisibilityTrace = ULockOnSettings::Get()->VisibilityTracePolicy != ELockOnVisibilityTracePolicy::Disabled;
	QueryAndScoreCandidates(ViewLocation, ViewRotation, false, bAllowVisibilityTrace, false, true, Candidates, TotalRegistered);
	RefreshDebugCandidatesFromLastQuery(Candidates);

	FLockOnCandidate BestCandidate;
	if (!ChooseCycleCandidate(Candidates, CycleDirection, BestCandidate) || !BestCandidate.TargetComponent.IsValid())
	{
		return false;
	}

	CommitLockState(BestCandidate.TargetComponent->GetOwner(), ELockOnBreakReason::None);
	SecondsSinceVisibilityLoss = -1.0f;
	return true;
}

bool ULockOnComponent::CanProcessServerCycleRequest(ELockOnCycleDirection CycleDirection)
{
	const float CooldownSeconds = ULockOnSettings::Get()->ServerCycleRequestCooldownSeconds;
	if (CooldownSeconds <= 0.0f)
	{
		return true;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}

	const float CurrentWorldTime = World->GetTimeSeconds();
	if (LastServerCycleRequestWorldTime >= 0.0f)
	{
		const float ElapsedSeconds = CurrentWorldTime - LastServerCycleRequestWorldTime;
		if (ElapsedSeconds < CooldownSeconds)
		{
			++RejectedCycleRequestCount;

			const float RemainingCooldownSeconds = CooldownSeconds - ElapsedSeconds;
			UE_LOG(LogLockOnSystem, Verbose,
				TEXT("LockOn cycle request rejected by server cooldown Direction=%s Remaining=%.3f"),
				*LockOnComponentPrivate::CycleDirectionToString(CycleDirection),
				RemainingCooldownSeconds);
			return false;
		}
	}

	LastServerCycleRequestWorldTime = CurrentWorldTime;
	return true;
}

void ULockOnComponent::ClearLockServer(ELockOnBreakReason BreakReason)
{
	CommitLockState(nullptr, BreakReason);
}

void ULockOnComponent::MaintainCurrentLockServer()
{
	if (!IsLockActive())
	{
		return;
	}

	++MaintainCheckCount;

	FVector ViewLocation;
	FRotator ViewRotation;
	if (!GetViewPoint(ViewLocation, ViewRotation))
	{
		ClearLockServer(ELockOnBreakReason::InvalidTarget);
		return;
	}

	ELockOnBreakReason BreakReason = ELockOnBreakReason::None;
	if (!IsTargetStillValid(GetCurrentTargetComponent(), ViewLocation, ViewRotation, BreakReason))
	{
		ClearLockServer(BreakReason);
	}
}

bool ULockOnComponent::GetViewPoint(FVector& OutViewLocation, FRotator& OutViewRotation) const
{
	if (const APawn* Pawn = GetPawn<APawn>())
	{
		if (const AController* Controller = Pawn->GetController())
		{
			Controller->GetPlayerViewPoint(OutViewLocation, OutViewRotation);
			return true;
		}

		OutViewLocation = Pawn->GetPawnViewLocation();
		OutViewRotation = Pawn->GetViewRotation();
		return true;
	}

	OutViewLocation = FVector::ZeroVector;
	OutViewRotation = FRotator::ZeroRotator;
	return false;
}

bool ULockOnComponent::EvaluateCandidate(ULockOnTargetComponent* TargetComponent, const FVector& ViewLocation, const FRotator& ViewRotation, bool bIsMaintainPass, bool bAllowVisibilityTrace, bool bApplyStickyBonus, FLockOnCandidate& OutCandidate)
{
	if (!TargetComponent)
	{
		return false;
	}

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn || !TargetComponent->CanBeLockedOnBy(Pawn))
	{
		return false;
	}

	const ULockOnSettings* Settings = ULockOnSettings::Get();
	const FVector FocusLocation = TargetComponent->GetLockOnFocusLocation();
	const FVector ToTarget = FocusLocation - ViewLocation;
	const float Distance = ToTarget.Size();
	if (Distance <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	if (FMath::Abs(FocusLocation.Z - ViewLocation.Z) > Settings->MaxHeightDelta)
	{
		return false;
	}

	const float Radius = bIsMaintainPass ? Settings->MaintainRadius : Settings->AcquireRadius;
	if (Distance > Radius)
	{
		return false;
	}

	const float HalfAngleDegrees = bIsMaintainPass ? Settings->MaintainHalfAngleDegrees : Settings->AcquireHalfAngleDegrees;
	const FVector ToTargetNormal = ToTarget / Distance;
	const float ViewDot = FVector::DotProduct(ViewRotation.Vector(), ToTargetNormal);
	const float RequiredDot = FMath::Cos(FMath::DegreesToRadians(HalfAngleDegrees));
	if (ViewDot < RequiredDot)
	{
		return false;
	}

	bool bVisibilityConfirmed = true;
	if (bAllowVisibilityTrace)
	{
		bVisibilityConfirmed = PerformVisibilityTrace(ViewLocation, FocusLocation, TargetComponent->GetOwner());
		if (!bVisibilityConfirmed)
		{
			return false;
		}
	}

	const FLockOnScoreWeights& Weights = Settings->ScoreWeights;
	OutCandidate.TargetComponent = TargetComponent;
	OutCandidate.FocusLocation = FocusLocation;
	OutCandidate.Distance = Distance;
	OutCandidate.ViewDot = ViewDot;
	OutCandidate.bVisibilityConfirmed = bVisibilityConfirmed;

	const float ScreenCenterNormalized = FMath::Clamp((ViewDot - RequiredDot) / FMath::Max(UE_SMALL_NUMBER, 1.0f - RequiredDot), 0.0f, 1.0f);
	const float DistanceNormalized = 1.0f - FMath::Clamp(Distance / FMath::Max(1.0f, Radius), 0.0f, 1.0f);
	const bool bIsCurrentTarget = (ReplicatedState.TargetActor == TargetComponent->GetOwner());

	OutCandidate.ScoreBreakdown.ScreenCenterScore = ScreenCenterNormalized * Weights.ScreenCenterWeight;
	OutCandidate.ScoreBreakdown.DistanceScore = DistanceNormalized * Weights.DistanceWeight;
	OutCandidate.ScoreBreakdown.VisibilityScore = (bAllowVisibilityTrace ? 1.0f : 0.5f) * Weights.VisibilityWeight;
	OutCandidate.ScoreBreakdown.StickyBonus = (bApplyStickyBonus && bIsCurrentTarget) ? Weights.StickyWeight : 0.0f;
	OutCandidate.ScoreBreakdown.TargetPriorityBonus = TargetComponent->GetTargetPriorityBonus() * Weights.TargetPriorityWeight;
	OutCandidate.ScoreBreakdown.TotalScore =
		OutCandidate.ScoreBreakdown.ScreenCenterScore +
		OutCandidate.ScoreBreakdown.DistanceScore +
		OutCandidate.ScoreBreakdown.VisibilityScore +
		OutCandidate.ScoreBreakdown.StickyBonus +
		OutCandidate.ScoreBreakdown.TargetPriorityBonus;

	return true;
}

bool ULockOnComponent::QueryAndScoreCandidates(const FVector& ViewLocation, const FRotator& ViewRotation, bool bIsMaintainPass, bool bAllowVisibilityTrace, bool bApplyStickyBonus, bool bUseSpatialQuery, TArray<FLockOnCandidate>& OutCandidates, int32& OutTotalRegistered)
{
	OutCandidates.Reset();

	ULockOnWorldSubsystem* WorldSubsystem = GetLockOnWorldSubsystem();
	if (!WorldSubsystem)
	{
		OutTotalRegistered = 0;
		return false;
	}

	const float QueryRadius = bIsMaintainPass ? ULockOnSettings::Get()->MaintainRadius : ULockOnSettings::Get()->AcquireRadius;
	TArray<ULockOnTargetComponent*> TargetComponents;
	WorldSubsystem->QueryTargets(ViewLocation, QueryRadius, bUseSpatialQuery, TargetComponents, OutTotalRegistered);
	++QueryCount;

	OutCandidates.Reserve(FMath::Min(TargetComponents.Num(), ULockOnSettings::Get()->MaxCandidates));

	for (ULockOnTargetComponent* TargetComponent : TargetComponents)
	{
		FLockOnCandidate Candidate;
		if (EvaluateCandidate(TargetComponent, ViewLocation, ViewRotation, bIsMaintainPass, bAllowVisibilityTrace, bApplyStickyBonus, Candidate))
		{
			OutCandidates.Add(MoveTemp(Candidate));
		}
	}

	OutCandidates.Sort([](const FLockOnCandidate& Left, const FLockOnCandidate& Right)
	{
		return LockOnComponentPrivate::CompareCandidatesDeterministically(Left, Right);
	});

	if (OutCandidates.Num() > ULockOnSettings::Get()->MaxCandidates)
	{
		OutCandidates.SetNum(ULockOnSettings::Get()->MaxCandidates, EAllowShrinking::No);
	}

	return OutCandidates.Num() > 0;
}

bool ULockOnComponent::ChooseBestCandidate(const TArray<FLockOnCandidate>& Candidates, FLockOnCandidate& OutBestCandidate) const
{
	if (Candidates.Num() == 0)
	{
		return false;
	}

	OutBestCandidate = Candidates[0];
	return true;
}

bool ULockOnComponent::ChooseCycleCandidate(const TArray<FLockOnCandidate>& Candidates, ELockOnCycleDirection CycleDirection, FLockOnCandidate& OutBestCandidate) const
{
	FVector ViewLocation;
	FRotator ViewRotation;
	if (!GetViewPoint(ViewLocation, ViewRotation))
	{
		return false;
	}

	const FVector ViewForward = ViewRotation.Vector();
	const FRotationMatrix ViewMatrix(ViewRotation);
	const FVector ViewRight = ViewMatrix.GetScaledAxis(EAxis::Y);
	const FVector ViewUp = ViewMatrix.GetScaledAxis(EAxis::Z);
	const FVector SignedAxis = LockOnComponentPrivate::IsHorizontalCycleDirection(CycleDirection) ? ViewRight : ViewUp;
	const AActor* CurrentTargetActor = ReplicatedState.TargetActor.Get();
	const ULockOnTargetComponent* CurrentTargetComponent = GetCurrentTargetComponent();
	if (!CurrentTargetActor || !CurrentTargetComponent)
	{
		return false;
	}

	const FVector CurrentDirection = (CurrentTargetComponent->GetLockOnFocusLocation() - ViewLocation).GetSafeNormal();
	const float CurrentAngleRadians = LockOnComponentPrivate::CalculateCameraSpaceAngleRadians(ViewForward, SignedAxis, CurrentDirection);

	bool bFoundCandidate = false;
	float BestAngularDistance = FLT_MAX;
	constexpr float MinimumCycleAngleRadians = 0.03f;
	const bool bUsePositiveDelta = LockOnComponentPrivate::IsPositiveCycleDirection(CycleDirection);

	for (const FLockOnCandidate& Candidate : Candidates)
	{
		const AActor* CandidateActor = Candidate.TargetComponent.IsValid() ? Candidate.TargetComponent->GetOwner() : nullptr;
		if (!CandidateActor || (CandidateActor == CurrentTargetActor))
		{
			continue;
		}

		const FVector DirectionToCandidate = (Candidate.FocusLocation - ViewLocation).GetSafeNormal();
		const float CandidateAngleRadians = LockOnComponentPrivate::CalculateCameraSpaceAngleRadians(ViewForward, SignedAxis, DirectionToCandidate);
		const float SignedDeltaRadians = FMath::FindDeltaAngleRadians(CurrentAngleRadians, CandidateAngleRadians);
		const float DirectionalDeltaRadians = bUsePositiveDelta ? SignedDeltaRadians : -SignedDeltaRadians;
		if (DirectionalDeltaRadians <= MinimumCycleAngleRadians)
		{
			continue;
		}

		if (!bFoundCandidate
			|| (DirectionalDeltaRadians < BestAngularDistance)
			|| (FMath::IsNearlyEqual(DirectionalDeltaRadians, BestAngularDistance) && LockOnComponentPrivate::CompareCandidatesDeterministically(Candidate, OutBestCandidate)))
		{
			OutBestCandidate = Candidate;
			BestAngularDistance = DirectionalDeltaRadians;
			bFoundCandidate = true;
		}
	}

	return bFoundCandidate;
}

bool ULockOnComponent::IsTargetStillValid(ULockOnTargetComponent* TargetComponent, const FVector& ViewLocation, const FRotator& ViewRotation, ELockOnBreakReason& OutBreakReason)
{
	OutBreakReason = ELockOnBreakReason::None;
	if (!TargetComponent || !ReplicatedState.TargetActor)
	{
		OutBreakReason = ELockOnBreakReason::TargetDestroyed;
		return false;
	}

	const bool bShouldRunMaintainTrace =
		(ULockOnSettings::Get()->VisibilityTracePolicy == ELockOnVisibilityTracePolicy::AcquireCycleAndLowRateMaintain) &&
		(MaintainVisibilityAccumulator >= ULockOnSettings::Get()->MaintainVisibilityCheckIntervalSeconds);

	FLockOnCandidate Candidate;
	if (EvaluateCandidate(TargetComponent, ViewLocation, ViewRotation, true, false, true, Candidate))
	{
		return true;
	}

	const APawn* Pawn = GetPawn<APawn>();
	if (!TargetComponent->CanBeLockedOnBy(Pawn))
	{
		OutBreakReason = ELockOnBreakReason::InvalidTarget;
		return false;
	}

	const FVector FocusLocation = TargetComponent->GetLockOnFocusLocation();
	const FVector ToTarget = FocusLocation - ViewLocation;
	const float Distance = ToTarget.Size();
	if (Distance > ULockOnSettings::Get()->MaintainRadius)
	{
		OutBreakReason = ELockOnBreakReason::OutOfRange;
		return false;
	}

	const float RequiredDot = FMath::Cos(FMath::DegreesToRadians(ULockOnSettings::Get()->MaintainHalfAngleDegrees));
	const float ViewDot = FVector::DotProduct(ViewRotation.Vector(), ToTarget.GetSafeNormal());
	if (ViewDot < RequiredDot)
	{
		OutBreakReason = ELockOnBreakReason::OutOfCone;
		return false;
	}

	if (bShouldRunMaintainTrace)
	{
		const bool bVisible = PerformVisibilityTrace(ViewLocation, FocusLocation, TargetComponent->GetOwner());
		MaintainVisibilityAccumulator = 0.0f;

		if (!bVisible)
		{
			if (SecondsSinceVisibilityLoss < 0.0f)
			{
				SecondsSinceVisibilityLoss = 0.0f;
			}
			else
			{
				SecondsSinceVisibilityLoss += ULockOnSettings::Get()->MaintainCheckIntervalSeconds;
			}

			if (SecondsSinceVisibilityLoss >= ULockOnSettings::Get()->BreakGraceSeconds)
			{
				OutBreakReason = ELockOnBreakReason::Occluded;
				return false;
			}
		}
		else
		{
			SecondsSinceVisibilityLoss = -1.0f;
		}
	}

	return true;
}

bool ULockOnComponent::PerformVisibilityTrace(const FVector& Start, const FVector& End, const AActor* TargetActor)
{
	++VisibilityTraceCount;

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(LockOnVisibilityTrace), false);
	if (const APawn* Pawn = GetPawn<APawn>())
	{
		Params.AddIgnoredActor(Pawn);
	}

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	return !bHit || (Hit.GetActor() == TargetActor);
}

void ULockOnComponent::CommitLockState(AActor* NewTargetActor, ELockOnBreakReason BreakReason)
{
	const AActor* PreviousTargetActor = ReplicatedState.TargetActor;

	ReplicatedState.TargetActor = NewTargetActor;
	ReplicatedState.bIsLocked = (NewTargetActor != nullptr);
	ReplicatedState.BreakReason = BreakReason;
	++ReplicatedState.Sequence;
	SecondsSinceVisibilityLoss = -1.0f;
	MaintainVisibilityAccumulator = 0.0f;

	if ((BreakReason != ELockOnBreakReason::None) && ((int32)BreakReason < (int32)ELockOnBreakReason::MAX))
	{
		++BreakReasonCounts[(int32)BreakReason];
	}

	if (PreviousTargetActor != NewTargetActor)
	{
		UE_LOG(LogLockOnSystem, Verbose, TEXT("LockOn commit target=%s reason=%s sequence=%u"), *GetNameSafe(NewTargetActor), *LockOnComponentPrivate::BreakReasonToString(BreakReason), ReplicatedState.Sequence);
	}

	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->ForceNetUpdate();
	}

	HandleLockStateChanged();
}

void ULockOnComponent::HandleLockStateChanged()
{
	RecordPreviewMismatchIfNeeded(ReplicatedState.TargetActor);

	if (IsLockActive())
	{
		LocalPreviewTargetActor = ReplicatedState.TargetActor;
	}
	else if (ShouldRunLocalPreview())
	{
		PreviewRefreshAccumulator = ULockOnSettings::Get()->PreviewRefreshIntervalSeconds;
	}
	else
	{
		LocalPreviewTargetActor.Reset();
	}
}

void ULockOnComponent::UpdateLocalPreview(float DeltaTime)
{
	if (IsLockActive())
	{
		return;
	}

	PreviewRefreshAccumulator += DeltaTime;
	if (PreviewRefreshAccumulator < ULockOnSettings::Get()->PreviewRefreshIntervalSeconds)
	{
		return;
	}

	PreviewRefreshAccumulator = 0.0f;

	FVector ViewLocation;
	FRotator ViewRotation;
	if (!GetViewPoint(ViewLocation, ViewRotation))
	{
		return;
	}

	TArray<FLockOnCandidate> Candidates;
	int32 TotalRegistered = 0;
	QueryAndScoreCandidates(ViewLocation, ViewRotation, false, false, false, true, Candidates, TotalRegistered);
	RefreshDebugCandidatesFromLastQuery(Candidates);

	FLockOnCandidate BestCandidate;
	LocalPreviewTargetActor = ChooseBestCandidate(Candidates, BestCandidate) && BestCandidate.TargetComponent.IsValid()
		? BestCandidate.TargetComponent->GetOwner()
		: nullptr;
}

void ULockOnComponent::UpdateLocalHighlight()
{
	AActor* CurrentTargetActor = ReplicatedState.TargetActor.Get();
	AActor* DesiredHighlightedActor = IsLockActive() ? CurrentTargetActor : LocalPreviewTargetActor.Get();
	SetHighlightedActor(DesiredHighlightedActor);
}

void ULockOnComponent::ClearLocalHighlight()
{
	SetHighlightedActor(nullptr);
}

void ULockOnComponent::SetHighlightedActor(AActor* NewHighlightedActor)
{
	if (HighlightedActor.Get() == NewHighlightedActor)
	{
		return;
	}

	for (const FLockOnPrimitiveHighlightState& Entry : HighlightedPrimitiveStates)
	{
		if (UPrimitiveComponent* PrimitiveComponent = Entry.PrimitiveComponent.Get())
		{
			PrimitiveComponent->SetRenderCustomDepth(Entry.bOriginalRenderCustomDepth);
			PrimitiveComponent->SetCustomDepthStencilValue(Entry.OriginalStencilValue);
		}
	}

	HighlightedPrimitiveStates.Reset();
	HighlightedActor = nullptr;

	if (!NewHighlightedActor || !ULockOnSettings::Get()->bEnablePreviewCustomDepth)
	{
		return;
	}

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(NewHighlightedActor);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!PrimitiveComponent)
		{
			continue;
		}

		FLockOnPrimitiveHighlightState& HighlightState = HighlightedPrimitiveStates.AddDefaulted_GetRef();
		HighlightState.PrimitiveComponent = PrimitiveComponent;
		HighlightState.bOriginalRenderCustomDepth = PrimitiveComponent->bRenderCustomDepth;
		HighlightState.OriginalStencilValue = PrimitiveComponent->CustomDepthStencilValue;

		PrimitiveComponent->SetRenderCustomDepth(true);
		PrimitiveComponent->SetCustomDepthStencilValue(ULockOnSettings::Get()->PreviewCustomDepthStencilValue);
	}

	HighlightedActor = NewHighlightedActor;
}

void ULockOnComponent::DrawDebugState()
{
	UWorld* World = GetWorld();
	const APawn* Pawn = GetPawn<APawn>();
	if (!World || !Pawn)
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	if (!GetViewPoint(ViewLocation, ViewRotation))
	{
		return;
	}

	DrawDebugString(World,
		Pawn->GetActorLocation() + FVector(0.0f, 0.0f, 125.0f),
		FString::Printf(TEXT("Lock=%s Target=%s Preview=%s Traces=%d Maintain=%d Rejects=%d Recoveries=%d"),
			IsLockActive() ? TEXT("On") : TEXT("Off"),
			*GetNameSafe(ReplicatedState.TargetActor),
			*GetNameSafe(LocalPreviewTargetActor.Get()),
			VisibilityTraceCount,
			MaintainCheckCount,
			RejectedCycleRequestCount,
			DelegateChainRecoveryCount),
		nullptr,
		FColor::Green,
		0.0f,
		true);

	const int32 MaxToDraw = FMath::Min(LastScoredCandidates.Num(), ULockOnSettings::Get()->MaxDebugCandidatesToDraw);
	for (int32 Index = 0; Index < MaxToDraw; ++Index)
	{
		const FLockOnCandidate& Candidate = LastScoredCandidates[Index];
		const AActor* TargetActor = Candidate.TargetComponent.IsValid() ? Candidate.TargetComponent->GetOwner() : nullptr;
		if (!TargetActor)
		{
			continue;
		}

		const FColor CandidateColor = (TargetActor == ReplicatedState.TargetActor) ? FColor::Yellow : FColor::Cyan;
		DrawDebugLine(World, ViewLocation, Candidate.FocusLocation, CandidateColor, false, 0.0f, 0, 0.75f);
		DrawDebugSphere(World, Candidate.FocusLocation, 20.0f, 8, CandidateColor, false, 0.0f);
		DrawDebugString(World,
			Candidate.FocusLocation + FVector(0.0f, 0.0f, 25.0f),
			FString::Printf(TEXT("%s T=%.2f S=%.2f D=%.2f V=%.2f K=%.2f P=%.2f"),
				*GetNameSafe(TargetActor),
				Candidate.ScoreBreakdown.TotalScore,
				Candidate.ScoreBreakdown.ScreenCenterScore,
				Candidate.ScoreBreakdown.DistanceScore,
				Candidate.ScoreBreakdown.VisibilityScore,
				Candidate.ScoreBreakdown.StickyBonus,
				Candidate.ScoreBreakdown.TargetPriorityBonus),
			nullptr,
			CandidateColor,
			0.0f,
			true);
	}
}

void ULockOnComponent::RefreshDebugCandidatesFromLastQuery(const TArray<FLockOnCandidate>& Candidates)
{
	LastScoredCandidates = Candidates;
}

bool ULockOnComponent::ShouldRunLocalPreview() const
{
	const APawn* Pawn = GetPawn<APawn>();
	return Pawn && Pawn->IsLocallyControlled();
}

bool ULockOnComponent::ShouldRunServerAuthority() const
{
	const APawn* Pawn = GetPawn<APawn>();
	return Pawn && (GetOwnerRole() == ROLE_Authority) && Pawn->IsPlayerControlled();
}

ULockOnWorldSubsystem* ULockOnComponent::GetLockOnWorldSubsystem() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<ULockOnWorldSubsystem>() : nullptr;
}

ULockOnTargetComponent* ULockOnComponent::GetCurrentTargetComponent() const
{
	return ReplicatedState.TargetActor ? ReplicatedState.TargetActor->FindComponentByClass<ULockOnTargetComponent>() : nullptr;
}

void ULockOnComponent::RecordPreviewMismatchIfNeeded(AActor* AuthoritativeTargetActor)
{
	if (!ShouldRunLocalPreview())
	{
		return;
	}

	AActor* PreviewActor = LocalPreviewTargetActor.Get();
	if (PreviewActor && (PreviewActor != AuthoritativeTargetActor))
	{
		++PreviewMismatchCount;
	}
}
