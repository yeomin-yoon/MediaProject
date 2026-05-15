import os
import unreal


OUTPUT_PATH = r"D:\UnrealProject\MediaProject\LyraStarterGame\Saved\Codex\inspect_melee_weapon_bp.txt"
LINES = []


def log(message):
    line = f"[WeaponBP] {message}"
    LINES.append(line)
    unreal.log(line)


def try_get(obj, prop):
    try:
        return obj.get_editor_property(prop)
    except Exception as exc:
        return f"<error:{exc}>"


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
    log(f"    trace_complex={try_get(profile, 'trace_complex')} b_trace_complex={try_get(profile, 'b_trace_complex')}")
    log(f"    ignore_owner_attached_actors={try_get(profile, 'ignore_owner_attached_actors')} b_ignore_owner_attached_actors={try_get(profile, 'b_ignore_owner_attached_actors')}")
    log(f"    stop_at_blocking_hit={try_get(profile, 'stop_at_blocking_hit')} b_stop_at_blocking_hit={try_get(profile, 'b_stop_at_blocking_hit')}")
    log(f"    trace_point_count={len(trace_points)}")
    for index, point in enumerate(trace_points):
        log(f"      [{index}] socket={point.socket_name} offset={point.local_offset}")


def main():
    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    try:
        bp = unreal.EditorAssetLibrary.load_asset("/Game/1dev/OS/Weapon/B_MeeleWeapon_Test")
        if not bp:
            raise RuntimeError("Failed to load B_MeeleWeapon_Test")

        generated_class = unreal.BlueprintEditorLibrary.generated_class(bp)
        cdo = unreal.get_default_object(generated_class)
        log(f"bp={bp.get_path_name()}")
        log(f"generated_class={generated_class.get_name()}")
        log(f"cdo={cdo.get_path_name()}")

        components = cdo.get_components_by_class(unreal.ActorComponent)
        log(f"component_count={len(components)}")
        for comp in components:
            log(f"  component={comp.get_name()} class={comp.get_class().get_name()}")
            for prop in ["static_mesh", "skeletal_mesh", "trace_source_id", "trace_source_component", "default_trace_profile"]:
                value = try_get(comp, prop)
                if prop == "default_trace_profile" and not isinstance(value, str):
                    dump_trace_profile(value)
                else:
                    log(f"    {prop}={value}")
    finally:
        with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
            handle.write("\n".join(LINES))


main()
