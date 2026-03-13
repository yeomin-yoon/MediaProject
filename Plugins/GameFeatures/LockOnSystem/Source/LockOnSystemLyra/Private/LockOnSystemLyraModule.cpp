#include "LockOnSystemLyraModule.h"

#include "Components/GameFrameworkComponentManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "LockOnComponent.h"
#include "LockOnLyraBridgeComponent.h"
#include "LockOnTargetComponent.h"
#include "LockOnSystemRuntimeLog.h"
#include "Character/LyraCharacter.h"

void FLockOnSystemLyraModule::StartupModule()
{
	GameInstanceStartHandle = FWorldDelegates::OnStartGameInstance.AddRaw(this, &FLockOnSystemLyraModule::HandleGameInstanceStart);
}

void FLockOnSystemLyraModule::ShutdownModule()
{
	if (GameInstanceStartHandle.IsValid())
	{
		FWorldDelegates::OnStartGameInstance.Remove(GameInstanceStartHandle);
		GameInstanceStartHandle.Reset();
	}

	ComponentRequestsByGameInstance.Empty();
}

void FLockOnSystemLyraModule::HandleGameInstanceStart(UGameInstance* GameInstance)
{
	if (!GameInstance || ComponentRequestsByGameInstance.Contains(GameInstance))
	{
		return;
	}

	UGameFrameworkComponentManager* ComponentManager = UGameInstance::GetSubsystem<UGameFrameworkComponentManager>(GameInstance);
	if (!ComponentManager)
	{
		return;
	}

	TArray<TSharedPtr<FComponentRequestHandle>>& Requests = ComponentRequestsByGameInstance.Add(GameInstance);
	Requests.Add(ComponentManager->AddComponentRequest(ALyraCharacter::StaticClass(), ULockOnComponent::StaticClass()));
	Requests.Add(ComponentManager->AddComponentRequest(ALyraCharacter::StaticClass(), ULockOnTargetComponent::StaticClass()));
	Requests.Add(ComponentManager->AddComponentRequest(ALyraCharacter::StaticClass(), ULockOnLyraBridgeComponent::StaticClass()));

	UE_LOG(LogLockOnSystem, Log, TEXT("LockOnSystemLyra registered lock-on component requests for %s"), *GetNameSafe(GameInstance));
}

IMPLEMENT_MODULE(FLockOnSystemLyraModule, LockOnSystemLyra)
