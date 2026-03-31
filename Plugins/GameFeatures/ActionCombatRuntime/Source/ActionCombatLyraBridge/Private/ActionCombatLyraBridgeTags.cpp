#include "ActionCombatLyraBridgeTags.h"

namespace ActionCombatLyraBridgeTags
{
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Type_Action_Dash, "Ability.Type.Action.Dash", "Lyra dash ability is currently active.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_State_Dodge, "Combat.State.Dodge", "Owner is currently performing a dash-based dodge.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_State_Dodge_IFrame, "Combat.State.Dodge.IFrame", "Owner is inside the dash-based dodge invulnerability window.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_State_Guard, "Combat.State.Guard", "Owner is currently in an active guard state.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_State_ForcedGuard, "Combat.State.ForcedGuard", "Owner is locked into a short forced-guard window after blocking.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_State_GuardBroken, "Combat.State.GuardBroken", "Owner guard resource has been depleted and guard is broken.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_GameplayEvent_GuardBlocked, "Combat.GameplayEvent.Guard.Blocked", "Gameplay event dispatched when an incoming hit is successfully blocked.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_GameplayEvent_GuardBroken, "Combat.GameplayEvent.Guard.Broken", "Gameplay event dispatched when an incoming hit breaks guard.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_DamageMultiplier, "SetByCaller.DamageMultiplier", "SetByCaller tag used by melee hit effects to scale damage by hurtbox or weapon rules.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_GuardDamage, "SetByCaller.GuardDamage", "SetByCaller tag used by guarded hits to expose stamina or guard pressure cost.");
}
