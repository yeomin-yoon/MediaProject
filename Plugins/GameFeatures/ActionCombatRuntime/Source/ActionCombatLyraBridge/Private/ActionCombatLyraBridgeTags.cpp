#include "ActionCombatLyraBridgeTags.h"

namespace ActionCombatLyraBridgeTags
{
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Type_Action_Dash, "Ability.Type.Action.Dash", "Lyra dash ability is currently active.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_State_Action, "Combat.State.Action", "Owner is currently performing an ActionCombat attack/action and should not accept normal movement input.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_State_Dodge, "Combat.State.Dodge", "Owner is currently performing a dash-based dodge.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_State_Dodge_IFrame, "Combat.State.Dodge.IFrame", "Owner is inside the dash-based dodge invulnerability window.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_State_Guard, "Combat.State.Guard", "Owner is currently in an active guard state.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_State_ForcedGuard, "Combat.State.ForcedGuard", "Owner is locked into a short forced-guard window after blocking.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_State_GuardBroken, "Combat.State.GuardBroken", "Owner guard resource has been depleted and guard is broken.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_GameplayEvent_GuardBlocked, "Combat.GameplayEvent.Guard.Blocked", "Gameplay event dispatched when an incoming hit is successfully blocked.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_GameplayEvent_GuardBroken, "Combat.GameplayEvent.Guard.Broken", "Gameplay event dispatched when an incoming hit breaks guard.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Attack_BaseDamage, "SetByCaller.Attack.BaseDamage", "Resolved weapon or fallback base damage before stat scaling and motion value are applied.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Attack_MotionValue, "SetByCaller.Attack.MotionValue", "Action-specific motion value authored in ActionCombat style data.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Attack_Strength, "SetByCaller.Attack.Strength", "Snapshot of the attacker's Strength stat at action start.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Attack_Dexterity, "SetByCaller.Attack.Dexterity", "Snapshot of the attacker's Dexterity stat at action start.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Attack_Intelligence, "SetByCaller.Attack.Intelligence", "Snapshot of the attacker's Intelligence stat at action start.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Attack_Faith, "SetByCaller.Attack.Faith", "Snapshot of the attacker's Faith stat at action start.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Attack_Arcane, "SetByCaller.Attack.Arcane", "Snapshot of the attacker's Arcane stat at action start.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Attack_CustomAttackPower, "SetByCaller.Attack.CustomAttackPower", "Snapshot of the attacker's project inventory AttackPower stat at action start.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Attack_StrengthScaling, "SetByCaller.Attack.StrengthScaling", "Weapon Strength scaling coefficient resolved from the equipped weapon definition.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Attack_DexterityScaling, "SetByCaller.Attack.DexterityScaling", "Weapon Dexterity scaling coefficient resolved from the equipped weapon definition.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Attack_IntelligenceScaling, "SetByCaller.Attack.IntelligenceScaling", "Weapon Intelligence scaling coefficient resolved from the equipped weapon definition.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Attack_FaithScaling, "SetByCaller.Attack.FaithScaling", "Weapon Faith scaling coefficient resolved from the equipped weapon definition.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Attack_ArcaneScaling, "SetByCaller.Attack.ArcaneScaling", "Weapon Arcane scaling coefficient resolved from the equipped weapon definition.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Attack_PoiseDamage, "SetByCaller.Attack.PoiseDamage", "Action-specific poise damage snapshot reserved for later stagger work.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Attack_BuildupMultiplier, "SetByCaller.Attack.BuildupMultiplier", "Action-specific buildup multiplier snapshot reserved for later status buildup work.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_DamageMultiplier, "SetByCaller.DamageMultiplier", "SetByCaller tag used by melee hit effects to scale damage by hurtbox or weapon rules.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_GuardDamage, "SetByCaller.GuardDamage", "SetByCaller tag used by guarded hits to expose stamina or guard pressure cost.");
}
