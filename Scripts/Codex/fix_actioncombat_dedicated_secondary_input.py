import os

import unreal


OUTPUT_PATH = r"D:\UnrealProject\MediaProject\LyraStarterGame\Saved\Codex\fix_actioncombat_dedicated_secondary_input.txt"
IMC_PATH = "/Game/Input/Mappings/IMC_Default"
PRIMARY_ACTION_PATH = "/Game/1dev/OS/IA_TestHero_Combat_Primary"
SECONDARY_ACTION_PATH = "/Game/1dev/OS/IA_TestHero_Combat_Secondary"
OLD_RMB_ACTION_PATH = "/Game/Input/Actions/IA_Weapon_Fire_Auto"
LINES = []


def log(message):
    LINES.append(message)
    unreal.log("[CodexSecondaryInput] " + message)


def write_output():
    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
        handle.write("\n".join(LINES))


def path_text(obj):
    if not obj:
        return "<none>"
    try:
        return obj.get_path_name()
    except Exception:
        return str(obj)


def key_name(key):
    getter = getattr(key, "get_editor_property", None)
    if callable(getter):
        for prop in ("key_name", "name"):
            try:
                value = getter(prop)
                if value:
                    return str(value)
            except Exception:
                pass
    return str(key)


def ensure_secondary_action():
    secondary = unreal.EditorAssetLibrary.load_asset(SECONDARY_ACTION_PATH)
    if secondary:
        log(f"secondary action already exists: {path_text(secondary)}")
        return secondary

    primary = unreal.EditorAssetLibrary.load_asset(PRIMARY_ACTION_PATH)
    if not primary:
        raise RuntimeError(f"missing primary action: {PRIMARY_ACTION_PATH}")

    secondary = unreal.EditorAssetLibrary.duplicate_asset(PRIMARY_ACTION_PATH, SECONDARY_ACTION_PATH)
    if not secondary:
        raise RuntimeError(f"failed to create secondary action: {SECONDARY_ACTION_PATH}")

    log(f"created secondary action: {path_text(secondary)}")
    return secondary


def main():
    imc = unreal.EditorAssetLibrary.load_asset(IMC_PATH)
    if not imc:
        raise RuntimeError(f"missing input mapping context: {IMC_PATH}")

    secondary = ensure_secondary_action()
    old_rmb_action = unreal.EditorAssetLibrary.load_asset(OLD_RMB_ACTION_PATH)

    mappings = list(imc.get_editor_property("mappings"))
    changed = False
    found_rmb_secondary = False

    for index, mapping in enumerate(mappings):
        action = mapping.get_editor_property("action")
        key = mapping.get_editor_property("key")
        current_key = key_name(key)
        current_action = path_text(action)
        log(f"[{index}] key={current_key} action={current_action}")

        is_rmb = "RightMouseButton" in current_key or "Right Mouse Button" in current_key
        if is_rmb and action == secondary:
            found_rmb_secondary = True
        elif is_rmb and (action == old_rmb_action or "IA_Weapon_Fire_Auto" in current_action):
            mapping.set_editor_property("action", secondary)
            found_rmb_secondary = True
            changed = True
            log(f"  changed RMB mapping to dedicated secondary at index {index}")

    if changed:
        imc.set_editor_property("mappings", mappings)

    if not found_rmb_secondary:
        add_mapping = getattr(imc, "map_key", None)
        if callable(add_mapping):
            right_mouse = unreal.Key()
            right_mouse.set_editor_property("key_name", "RightMouseButton")
            add_mapping(secondary, right_mouse)
            changed = True
            log("  added RMB mapping to dedicated secondary")
        else:
            raise RuntimeError("RMB secondary mapping was missing and map_key was unavailable")

    if changed:
        secondary_saved = unreal.EditorAssetLibrary.save_loaded_asset(secondary)
        imc_saved = unreal.EditorAssetLibrary.save_loaded_asset(imc)
        log(f"save results: secondary={secondary_saved} imc_default={imc_saved}")
        if not secondary_saved or not imc_saved:
            raise RuntimeError("failed to save secondary input assets")
    else:
        log("no changes needed")


try:
    main()
finally:
    write_output()
