#include "ActionCombatAppearanceComponent.h"

#include "ActionCombatAppearanceData.h"
#include "ActionCombatRuntimeLog.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"

UActionCombatAppearanceComponent::UActionCombatAppearanceComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UActionCombatAppearanceComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoApplySourceMeshVisibilityRules)
    {
        ApplySourceMeshVisibilityRules();
    }
}

void UActionCombatAppearanceComponent::SetAppearanceData(UActionCombatAppearanceData* NewAppearanceData)
{
    AppearanceData = NewAppearanceData;
}

USkeletalMeshComponent* UActionCombatAppearanceComponent::GetPrimaryVisualMesh() const
{
    if (USkeletalMeshComponent* ExplicitMesh = ResolveMeshFromReference(PrimaryVisualMeshComponent))
    {
        return ExplicitMesh;
    }

    return ResolvePrimaryVisualMeshByFallback();
}

USkeletalMeshComponent* UActionCombatAppearanceComponent::GetSourceMesh() const
{
    if (USkeletalMeshComponent* ExplicitMesh = ResolveMeshFromReference(SourceMeshComponent))
    {
        return ExplicitMesh;
    }

    return ResolveSourceMeshByFallback();
}

FGameplayTagContainer UActionCombatAppearanceComponent::GetAppearanceTags() const
{
    return AppearanceData ? AppearanceData->GetAppearanceTags() : FGameplayTagContainer();
}

void UActionCombatAppearanceComponent::ApplySourceMeshVisibilityRules()
{
    USkeletalMeshComponent* SourceMesh = GetSourceMesh();
    if (!SourceMesh)
    {
        UE_LOG(LogActionCombatRuntime, Verbose, TEXT("AppearanceComponent on %s could not resolve a source mesh to hide."), *GetPathNameSafe(GetOwner()));
        return;
    }

    SourceMesh->SetHiddenInGame(bHideSourceMeshInGame);

    if (bDisableSourceMeshShadow)
    {
        SourceMesh->SetCastShadow(false);
        SourceMesh->bCastHiddenShadow = false;
    }

    if (bForceSourceMeshAlwaysTickPose)
    {
        SourceMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    }
}

bool UActionCombatAppearanceComponent::TryResolveAccessoryAttachment(const FGameplayTag& SlotTag, USceneComponent*& OutAttachComponent, FName& OutAttachSocket) const
{
    OutAttachComponent = nullptr;
    OutAttachSocket = NAME_None;

    const FActionCombatAppearanceSlotDefinition* SlotDefinition = AppearanceData ? AppearanceData->FindSlotDefinition(SlotTag) : nullptr;
    if (SlotDefinition)
    {
        OutAttachSocket = SlotDefinition->AttachSocket;

        if (!SlotDefinition->TargetComponentName.IsNone())
        {
            OutAttachComponent = ResolveSceneComponentByName(SlotDefinition->TargetComponentName);
        }
    }

    if (!OutAttachComponent)
    {
        OutAttachComponent = GetPrimaryVisualMesh();
    }

    return OutAttachComponent != nullptr;
}

USkeletalMeshComponent* UActionCombatAppearanceComponent::ResolveMeshFromReference(const FComponentReference& ComponentReference) const
{
    if (AActor* Owner = GetOwner())
    {
        return Cast<USkeletalMeshComponent>(ComponentReference.GetComponent(Owner));
    }

    return nullptr;
}

USceneComponent* UActionCombatAppearanceComponent::ResolveSceneComponentByName(FName ComponentName) const
{
    if (!GetOwner() || ComponentName.IsNone())
    {
        return nullptr;
    }

    TInlineComponentArray<USceneComponent*> SceneComponents(GetOwner());
    for (USceneComponent* SceneComponent : SceneComponents)
    {
        if (IsValid(SceneComponent) && SceneComponent->GetFName() == ComponentName)
        {
            return SceneComponent;
        }
    }

    return nullptr;
}

USkeletalMeshComponent* UActionCombatAppearanceComponent::ResolveSourceMeshByFallback() const
{
    if (AppearanceData && !AppearanceData->GetDefaultSourceMeshComponentName().IsNone())
    {
        if (USkeletalMeshComponent* MeshByName = Cast<USkeletalMeshComponent>(ResolveSceneComponentByName(AppearanceData->GetDefaultSourceMeshComponentName())))
        {
            return MeshByName;
        }
    }

    if (const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
    {
        return OwnerCharacter->GetMesh();
    }

    TInlineComponentArray<USkeletalMeshComponent*> SkeletalMeshes(GetOwner());
    return SkeletalMeshes.Num() > 0 ? SkeletalMeshes[0] : nullptr;
}

USkeletalMeshComponent* UActionCombatAppearanceComponent::ResolvePrimaryVisualMeshByFallback() const
{
    if (AppearanceData && !AppearanceData->GetDefaultPrimaryVisualMeshComponentName().IsNone())
    {
        if (USkeletalMeshComponent* MeshByName = Cast<USkeletalMeshComponent>(ResolveSceneComponentByName(AppearanceData->GetDefaultPrimaryVisualMeshComponentName())))
        {
            return MeshByName;
        }
    }

    const USkeletalMeshComponent* SourceMesh = GetSourceMesh();

    TInlineComponentArray<USkeletalMeshComponent*> SkeletalMeshes(GetOwner());
    for (USkeletalMeshComponent* SkeletalMesh : SkeletalMeshes)
    {
        if (IsValid(SkeletalMesh) && SkeletalMesh != SourceMesh)
        {
            return SkeletalMesh;
        }
    }

    return const_cast<USkeletalMeshComponent*>(SourceMesh);
}
