import os
import unreal


OUTPUT_PATH = r"D:\UnrealProject\MediaProject\LyraStarterGame\Saved\Codex\inspect_current_melee_setup.txt"
LINES = []


def log(message):
    line = f"[CurrentMelee] {message}"
    LINES.append(line)
    unreal.log(line)


def try_get(obj, prop):
    try:
        return obj.get_editor_property(prop)
    except Exception as exc:
        return f"<error:{exc}>"


def fmt_asset(obj):
    if not obj:
        return "<none>"
    try:
        return obj.get_path_name()
    except Exception:
        return str(obj)


def dump_trace_profile(profile):
    try:
        trace_points = profile.trace_points
    except Exception as exc:
        log(f"    trace profile read failed: {exc}")
        return

    log(f"    sweep_radius={profile.sweep_radius}")
    log(f"    trace_channel={profile.trace_channel}")
    log(f"    max_hit_results_per_tick={profile.max_hit_results_per_tick}")
    log(f"    max_unique_targets_per_window={profile.max_unique_targets_per_window}")
    log(f"    b_trace_complex={profile.b_trace_complex}")
    log(f"    b_ignore_owner_attached_actors={profile.b_ignore_owner_attached_actors}")
    log(f"    b_stop_at_blocking_hit={profile.b_stop_at_blocking_hit}")
    log(f"    trace_point_count={len(trace_points)}")
    for index, point in enumerate(trace_points):
        log(f"      [{index}] socket={point.socket_name} offset={point.local_offset}")


def inspect_style():
    asset = unreal.EditorAssetLibrary.load_asset("/Game/1dev/OS/PrimaryAttack")
    if not asset:
        raise RuntimeError("Failed to load PrimaryAttack")

    actions = try_get(asset, "actions")
    transitions = try_get(asset, "transitions")
    log(f"style={fmt_asset(asset)} action_count={len(actions)} transition_count={len(transitions)}")

    for index, action in enumerate(actions):
        log(f"  action[{index}] tag={action.action_tag}")
        log(f"    montage={fmt_asset(action.montage)}")
        log(f"    play_rate={action.base_play_rate} queue={action.queue_window_starts_at_normalized_time}->{action.queue_window_closes_at_normalized_time} commit={action.chain_commit_at_normalized_time}")
        log(f"    trace_source_id={action.trace_source_id} hit_window={action.hit_window_name}")
        log(f"    motion={action.motion_value} poise={action.poise_damage} buildup={action.buildup_multiplier}")
        log(f"    allow_dodge_cancel={try_get(action, 'allow_dodge_cancel')} b_allow_dodge_cancel={try_get(action, 'b_allow_dodge_cancel')}")
        log(f"    dodge_cancel_time={try_get(action, 'dodge_cancel_starts_at_normalized_time')}")
        adv = action.attack_advance
        log(f"    advance enabled={try_get(adv, 'enabled')} b_enabled={try_get(adv, 'b_enabled')} distance={adv.distance} start={adv.start_normalized_time} end={adv.end_normalized_time} curve={adv.curve_exponent}")
        log(f"    resource_costs={len(action.resource_costs)}")

    for index, transition in enumerate(transitions):
        log(f"  transition[{index}] from={transition.from_action_tag} command={transition.command_tag} to={transition.to_action_tag}")
        log(f"    requires_focus_active={transition.b_requires_focus_active} requires_focus_inactive={transition.b_requires_focus_inactive}")
        log(f"    required_held={transition.required_held_input_tags}")
        log(f"    blocked_held={transition.blocked_held_input_tags}")


def inspect_weapon_blueprint():
    bp = unreal.EditorAssetLibrary.load_asset("/Game/1dev/OS/Weapon/B_MeeleWeapon_Test")
    if not bp:
        raise RuntimeError("Failed to load B_MeeleWeapon_Test")

    generated_class = unreal.BlueprintEditorLibrary.generated_class(bp)
    cdo = unreal.get_default_object(generated_class)
    log(f"weapon_bp={fmt_asset(bp)} generated_class={generated_class.get_name()} cdo={fmt_asset(cdo)}")

    components = cdo.get_components_by_class(unreal.ActorComponent)
    log(f"  cdo_component_count={len(components)}")
    for comp in components:
        log(f"  component={comp.get_name()} class={comp.get_class().get_name()}")
        for prop in ["static_mesh", "skeletal_mesh", "trace_source_id", "trace_source_component", "default_trace_profile"]:
            value = try_get(comp, prop)
            if prop == "default_trace_profile" and not isinstance(value, str):
                dump_trace_profile(value)
            else:
                log(f"    {prop}={value}")


def main():
    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    try:
        inspect_style()
        inspect_weapon_blueprint()
    finally:
        with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
            handle.write("\n".join(LINES))


main()
