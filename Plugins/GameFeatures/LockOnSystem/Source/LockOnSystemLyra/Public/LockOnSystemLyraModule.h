#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class UGameInstance;

class FLockOnSystemLyraModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void HandleGameInstanceStart(UGameInstance* GameInstance);

private:
	FDelegateHandle GameInstanceStartHandle;
	TMap<TWeakObjectPtr<UGameInstance>, TArray<TSharedPtr<struct FComponentRequestHandle>>> ComponentRequestsByGameInstance;
};
