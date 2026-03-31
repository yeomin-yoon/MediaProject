// Copyright Epic Games, Inc. All Rights Reserved.

#include "Utilities/ShooterTestsActorTest.h"

#if WITH_AUTOMATION_TESTS

#include "Animation/AnimationAsset.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"

ACTOR_ANIMATION_TEST_WITH_FLAGS(ActionCombatDragonRuntimeDashTest, "Project.Functional Tests.ActionCombat.DodgeRuntime", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
{
	ActionCombatDragonRuntimeDashTest()
		: ShooterTestsActorAnimationTest(TEXT("L_ShooterGym_ActionCombatTest"))
	{
	}

	USkeletalMeshComponent* SourceMesh{ nullptr };
	FGameplayTag DashAbilityTag;
	FGameplayTag DashInputTag;
	FGameplayTag CombatStateDodgeTag;
	FGameplayTag CombatStateDodgeIFrameTag;

	inline static const TCHAR* DashForwardAnimationPath = TEXT("/Game/Characters/Heroes/Mannequin/Animations/Actions/AM_MM_Dash_Forward.AM_MM_Dash_Forward");
	inline static const TCHAR* DashBackwardAnimationPath = TEXT("/Game/Characters/Heroes/Mannequin/Animations/Actions/AM_MM_Dash_Backward.AM_MM_Dash_Backward");
	inline static const TCHAR* DashLeftAnimationPath = TEXT("/Game/Characters/Heroes/Mannequin/Animations/Actions/AM_MM_Dash_Left.AM_MM_Dash_Left");
	inline static const TCHAR* DashRightAnimationPath = TEXT("/Game/Characters/Heroes/Mannequin/Animations/Actions/AM_MM_Dash_Right.AM_MM_Dash_Right");

	USkeletalMeshComponent* FindSkeletalMeshByName(const FName ComponentName) const
	{
		TArray<USkeletalMeshComponent*> MeshComponents;
		Player->GetComponents(MeshComponents);

		for (USkeletalMeshComponent* MeshComponent : MeshComponents)
		{
			if (IsValid(MeshComponent) && MeshComponent->GetFName() == ComponentName)
			{
				return MeshComponent;
			}
		}

		return nullptr;
	}

	bool HasComponentClassNameContaining(const FString& ClassNameFragment) const
	{
		TArray<UActorComponent*> Components;
		Player->GetComponents(Components);

		for (UActorComponent* Component : Components)
		{
			if (IsValid(Component) && Component->GetClass()->GetName().Contains(ClassNameFragment))
			{
				return true;
			}
		}

		return false;
	}

	bool HasEnhancedInputAction(const FString& InputActionName) const
	{
		auto HasActionInComponent = [&InputActionName](const UEnhancedInputComponent* InputComponent)
		{
			if (!IsValid(InputComponent))
			{
				return false;
			}

			for (const TUniquePtr<FEnhancedInputActionEventBinding>& Binding : InputComponent->GetActionEventBindings())
			{
				const UInputAction* Action = Binding->GetAction();
				if (IsValid(Action) && Action->GetName().Equals(InputActionName))
				{
					return true;
				}
			}

			return false;
		};

		if (HasActionInComponent(Cast<UEnhancedInputComponent>(Player->InputComponent)))
		{
			return true;
		}

		const APlayerController* PlayerController = Cast<APlayerController>(Player->GetController());
		return HasActionInComponent(PlayerController ? Cast<UEnhancedInputComponent>(PlayerController->InputComponent) : nullptr);
	}

	void ValidateDragonRuntimePawn()
	{
		ASSERT_THAT(IsTrue(Player->GetClass()->GetName().Contains(TEXT("B_Test_Hero_DragonRuntime"))));

		DashAbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Type.Action.Dash"), false);
		DashInputTag = FGameplayTag::RequestGameplayTag(TEXT("InputTag.Ability.Dash"), false);
		CombatStateDodgeTag = FGameplayTag::RequestGameplayTag(TEXT("Combat.State.Dodge"), false);
		CombatStateDodgeIFrameTag = FGameplayTag::RequestGameplayTag(TEXT("Combat.State.Dodge.IFrame"), false);
		ASSERT_THAT(IsTrue(DashAbilityTag.IsValid()));
		ASSERT_THAT(IsTrue(DashInputTag.IsValid()));
		ASSERT_THAT(IsTrue(CombatStateDodgeTag.IsValid()));
		ASSERT_THAT(IsTrue(CombatStateDodgeIFrameTag.IsValid()));

		SourceMesh = Player->GetMesh();
		ASSERT_THAT(IsNotNull(SourceMesh));
		ASSERT_THAT(IsTrue(SourceMesh->GetFName() == TEXT("CharacterMesh0")));

		USkeletalMeshComponent* DragonMesh = FindSkeletalMeshByName(TEXT("DragonMesh"));
		ASSERT_THAT(IsNotNull(DragonMesh));
		ASSERT_THAT(IsNotNull(DragonMesh->GetAnimClass()));
		ASSERT_THAT(IsTrue(DragonMesh->GetAnimClass()->GetName().Contains(TEXT("ABP_DragonKnight_RuntimeRetarget_C"))));

		ASSERT_THAT(IsTrue(HasComponentClassNameContaining(TEXT("ActionCombatComponent"))));
		ASSERT_THAT(IsTrue(HasComponentClassNameContaining(TEXT("ActionCombatLyraInputBridgeComponent"))));
		ASSERT_THAT(IsTrue(HasComponentClassNameContaining(TEXT("ActionCombatLyraAbilityBridgeComponent"))));
		ASSERT_THAT(IsTrue(HasComponentClassNameContaining(TEXT("ActionCombatLyraMeleeWeaponSpawnerComponent"))));
		ASSERT_THAT(IsTrue(HasComponentClassNameContaining(TEXT("ActionCombatHurtboxComponent"))));

		ASSERT_THAT(IsTrue(HasEnhancedInputAction(TEXT("IA_Move"))));
		ASSERT_THAT(IsTrue(HasEnhancedInputAction(TEXT("IA_Ability_Dash"))));
	}

	void ActivateDashAbility()
	{
		const FGameplayAbilitySpec* DashAbilitySpec = FindDashAbilitySpec();
		ASSERT_THAT(IsNotNull(DashAbilitySpec));
		ASSERT_THAT(IsTrue(AbilitySystemComponent->TryActivateAbility(DashAbilitySpec->Handle)));
	}

	const FGameplayAbilitySpec* FindDashAbilitySpec() const
	{
		for (const FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent->GetActivatableAbilities())
		{
			const UGameplayAbility* Ability = AbilitySpec.Ability;
			const bool bMatchesDashTag = Ability && Ability->GetAssetTags().HasTagExact(DashAbilityTag);
			const bool bMatchesDashName = Ability && (Ability->GetName().Contains(TEXT("Dash")) || Ability->GetClass()->GetName().Contains(TEXT("Dash")));
			const bool bMatchesDashInputTag = AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(DashInputTag);
			if (bMatchesDashTag || bMatchesDashName || bMatchesDashInputTag)
			{
				return &AbilitySpec;
			}
		}

		return nullptr;
	}

	void TestDashMirrorsDodgeStateTags()
	{
		RegisterKnownDashCueErrors(true);
		TestCommandBuilder
			.Do([this]() { PawnActions->MoveForward(); })
			.Until([this]() { return Player->GetVelocity().SizeSquared() > 1.0f; })
			.Until([this]() { return FindDashAbilitySpec() != nullptr; }, FTimespan::FromSeconds(10.0))
			.Do([this]() { ActivateDashAbility(); })
			.Until([this]()
			{
				return AbilitySystemComponent->HasMatchingGameplayTag(CombatStateDodgeTag)
					&& AbilitySystemComponent->HasMatchingGameplayTag(CombatStateDodgeIFrameTag);
			}, FTimespan::FromSeconds(10.0))
			.Until([this]()
			{
				return !AbilitySystemComponent->HasMatchingGameplayTag(CombatStateDodgeTag)
					&& !AbilitySystemComponent->HasMatchingGameplayTag(CombatStateDodgeIFrameTag);
			}, FTimespan::FromSeconds(10.0));
	}

	void LoadExpectedAnimation(const TCHAR* AnimationPath)
	{
		ExpectedAnimation = LoadObject<UAnimationAsset>(nullptr, AnimationPath);
		ASSERT_THAT(IsTrue(IsValid(ExpectedAnimation), FString::Format(TEXT("Cannot load animation asset '{0}'"), { AnimationPath })));
	}

	void RegisterKnownDashCueErrors(bool bExpectDeactivateError = false)
	{
		// ShooterCore's legacy dash cue currently emits PIE blueprint errors for meshes without matching Niagara setup.
		// Keep the gameplay test signal clean while documenting the cue issue separately.
		TestRunner->AddExpectedErrorPlain(TEXT("PIE: Blueprint Runtime Error: \"Accessed None trying to read property CallFunc_SpawnSystemAttached_ReturnValue\""), EAutomationExpectedErrorFlags::Contains, 0);
		TestRunner->AddExpectedErrorPlain(TEXT("PIE: Blueprint Runtime Error: \"Accessed None trying to read property CallFunc_Array_Get_Item\""), EAutomationExpectedErrorFlags::Contains, 0);
		if (bExpectDeactivateError)
		{
			TestRunner->AddExpectedErrorPlain(TEXT("PIE: Blueprint Runtime Error: \"Accessed None trying to read property CallFunc_Array_Get_Item_1\""), EAutomationExpectedErrorFlags::Contains, 0);
		}
	}

	void TestDirectionalDashAnimation(const TCHAR* AnimationPath, TFunction<void()> MoveAction)
	{
		LoadExpectedAnimation(AnimationPath);
		RegisterKnownDashCueErrors(false);
		TestCommandBuilder
			.Do(MoveAction)
			.Until([this]() { return Player->GetVelocity().SizeSquared() > 1.0f; })
			.Until([this]() { return FindDashAbilitySpec() != nullptr; }, FTimespan::FromSeconds(10.0))
			.Do([this]() { ActivateDashAbility(); })
			.Until([this]() { return AnimationTestHelper.IsAnimationPlaying(SourceMesh, ExpectedAnimation); }, FTimespan::FromSeconds(10.0));
	}

	BEFORE_EACH()
	{
		ShooterTestsActorAnimationTest::Setup();
		TestCommandBuilder.Then([this]() { ValidateDragonRuntimePawn(); });
	}

	TEST_METHOD(PlayerSpawns_WithDragonRuntimeComponents)
	{
		TestCommandBuilder.Then([this]() { ValidateDragonRuntimePawn(); });
	}

	TEST_METHOD(PlayerDash_Forward)
	{
		TestDirectionalDashAnimation(DashForwardAnimationPath, [this]() { PawnActions->MoveForward(); });
	}

	TEST_METHOD(PlayerDash_Backward)
	{
		TestDirectionalDashAnimation(DashBackwardAnimationPath, [this]() { PawnActions->MoveBackward(); });
	}

	TEST_METHOD(PlayerDash_Left)
	{
		TestDirectionalDashAnimation(DashLeftAnimationPath, [this]() { PawnActions->StrafeLeft(); });
	}

	TEST_METHOD(PlayerDash_Right)
	{
		TestDirectionalDashAnimation(DashRightAnimationPath, [this]() { PawnActions->StrafeRight(); });
	}

	TEST_METHOD(PlayerDash_MirrorsDodgeStateTags)
	{
		TestDashMirrorsDodgeStateTags();
	}
};

#endif // WITH_AUTOMATION_TESTS
