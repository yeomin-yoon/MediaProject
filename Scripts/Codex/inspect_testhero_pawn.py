import os
import unreal


OUTPUT_PATH = r"D:\UnrealProject\MediaProject\LyraStarterGame\Saved\Codex\inspect_testhero_pawn.txt"
LINES = []


def log(message):
    line = f"[CodexPawn] {message}"
    LINES.append(line)
    unreal.log(line)


def try_get(obj, prop):
    try:
        return obj.get_editor_property(prop)
    except Exception as exc:
        return f"<error:{exc}>"


def dump_component(comp):
    log(f"component name={comp.get_name()} class={comp.get_class().get_name()}")
    interesting = [
        "style",
        "weapon_actor_class",
        "attach_socket",
        "relative_attach_transform",
        "input_bindings",
        "b_log_binding_flow",
    ]
    for prop in interesting:
        value = try_get(comp, prop)
        if isinstance(value, list):
            log(f"  {prop} count={len(value)}")
            for index, item in enumerate(value[:12]):
                log(f"    [{index}] {item}")
        else:
            log(f"  {prop}={value}")


def main():
    pawn_data = unreal.EditorAssetLibrary.load_asset("/Game/1dev/OS/TestHeroData_ActionCombatOnly")
    log(f"pawn_data class={pawn_data.get_class().get_name()}")

    for prop in ["pawn_class", "ability_sets", "input_config", "default_camera_mode"]:
        log(f"pawn_data.{prop}={try_get(pawn_data, prop)}")

    pawn_class = try_get(pawn_data, "pawn_class")
    if isinstance(pawn_class, str):
        log("pawn_class not resolved")
    else:
        log(f"pawn_class class_name={pawn_class.get_name()}")
        cdo = unreal.get_default_object(pawn_class)
        log(f"cdo={cdo} cdo_class={cdo.get_class().get_name()}")
        components = cdo.get_components_by_class(unreal.ActorComponent)
        log(f"component_count={len(components)}")
        for comp in components:
            dump_component(comp)

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
        handle.write("\n".join(LINES))


main()
