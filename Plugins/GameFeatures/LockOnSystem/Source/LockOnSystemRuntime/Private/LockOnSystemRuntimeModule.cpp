#include "LockOnSystemRuntimeModule.h"

#include "LockOnComponent.h"
#include "LockOnSystemRuntimeLog.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY(LogLockOnSystem);

namespace LockOnConsole
{
	static ULockOnComponent* FindLocalLockOnComponent(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		APlayerController* PlayerController = World->GetFirstPlayerController();
		APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
		return Pawn ? Pawn->FindComponentByClass<ULockOnComponent>() : nullptr;
	}

	static void ExecLockOnCommand(const TArray<FString>& Args, UWorld* World)
	{
		ULockOnComponent* LockOnComponent = FindLocalLockOnComponent(World);
		if (!LockOnComponent)
		{
			UE_LOG(LogLockOnSystem, Warning, TEXT("LockOn: no local lock-on component found."));
			return;
		}

		if (Args.Num() == 0)
		{
			LockOnComponent->DumpDebugState();
			return;
		}

		const FString Verb = Args[0].ToLower();
		if (Verb == TEXT("toggle"))
		{
			LockOnComponent->RequestToggleLock();
			return;
		}

		if (Verb == TEXT("left"))
		{
			LockOnComponent->RequestCycleLeft();
			return;
		}

		if (Verb == TEXT("right"))
		{
			LockOnComponent->RequestCycleRight();
			return;
		}

		if (Verb == TEXT("clear"))
		{
			LockOnComponent->RequestClearLock();
			return;
		}

		if (Verb == TEXT("debug"))
		{
			const bool bEnableDebug = (Args.Num() < 2) || !Args[1].Equals(TEXT("0"), ESearchCase::CaseSensitive);
			LockOnComponent->SetDebugDrawEnabled(bEnableDebug);
			return;
		}

		if (Verb == TEXT("benchmark"))
		{
			const int32 Iterations = (Args.Num() >= 2) ? FMath::Max(1, FCString::Atoi(*Args[1])) : 100;
			LockOnComponent->RunBenchmark(Iterations);
			return;
		}

		if (Verb == TEXT("dump"))
		{
			LockOnComponent->DumpDebugState();
			return;
		}

		UE_LOG(LogLockOnSystem, Warning, TEXT("LockOn commands: toggle | left | right | clear | debug [0|1] | benchmark [iterations] | dump"));
	}

	static FAutoConsoleCommandWithWorldAndArgs LockOnCommand(
		TEXT("LockOn"),
		TEXT("LockOn commands: toggle | left | right | clear | debug [0|1] | benchmark [iterations] | dump"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ExecLockOnCommand));
}

void FLockOnSystemRuntimeModule::StartupModule()
{
}

void FLockOnSystemRuntimeModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FLockOnSystemRuntimeModule, LockOnSystemRuntime)
