#include "ActionCombatRuntimeTags.h"

namespace ActionCombatRuntimeTags
{
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_State_Reaction, "Combat.State.Reaction", "Owner is inside a temporary reaction state and cannot start new combat actions.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_State_Reaction_LightHit, "Combat.State.Reaction.LightHit", "Owner is inside a short light-hit reaction state.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_State_Reaction_HeavyHit, "Combat.State.Reaction.HeavyHit", "Owner is inside a heavier stagger state caused by poise break.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_State_Reaction_Knockdown, "Combat.State.Reaction.Knockdown", "Owner has been knocked down by a very heavy hit or repeated poise breaks.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_State_Reaction_GetUp, "Combat.State.Reaction.GetUp", "Owner is recovering from knockdown and should not start new combat actions yet.");
}
