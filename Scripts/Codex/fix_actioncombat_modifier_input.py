import os

import unreal


OUTPUT_PATH = r"D:\UnrealProject\MediaProject\LyraStarterGame\Saved\Codex\fix_actioncombat_modifier_input.txt"
IMC_PATH = "/Game/Input/Mappings/IMC_Default"
PRIMARY_ACTION_PATH = "/Game/1dev/OS/IA_TestHero_Combat_Primary"
MODIFIER_ACTION_PATH = "/Game/1dev/OS/IA_TestHero_Combat_Modifier"
CROUCH_ACTION_PATH = "/Game/Input/Actions/IA_Crouch"

LINES = []


def log(message):
    LINES.append(message)
    unreal.log("[CodexModifierInput] " + message)


def write_output():
    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
        handle.write("\n".join(LINES))


def asset_name(asset):
    return asset.get_path_name() if asset else "<none>"


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

    for attr in ("get_name", "to_string"):
        fn = getattr(key, attr, None)
        if callable(fn):
            try:
                value = fn()
                if value:
                    return str(value)
            except Exception:
                pass
    return str(key)


def ensure_modifier_action():
    modifier = unreal.EditorAssetLibrary.load_asset(MODIFIER_ACTION_PATH)
    if modifier:
        log(f"modifier action already exists: {asset_name(modifier)}")
        return modifier

    primary = unreal.EditorAssetLibrary.load_asset(PRIMARY_ACTION_PATH)
    if not primary:
        raise RuntimeError(f"missing primary input action: {PRIMARY_ACTION_PATH}")

    modifier = unreal.EditorAssetLibrary.duplicate_asset(PRIMARY_ACTION_PATH, MODIFIER_ACTION_PATH)
    if not modifier:
        raise RuntimeError(f"failed to duplicate {PRIMARY_ACTION_PATH} to {MODIFIER_ACTION_PATH}")

    log(f"created modifier action: {asset_name(modifier)}")
    return modifier


def main():
    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)

    imc = unreal.EditorAssetLibrary.load_asset(IMC_PATH)
    crouch = unreal.EditorAssetLibrary.load_asset(CROUCH_ACTION_PATH)
    modifier = ensure_modifier_action()

    if not imc:
        raise RuntimeError(f"missing input mapping context: {IMC_PATH}")
    if not crouch:
        raise RuntimeError(f"missing crouch input action: {CROUCH_ACTION_PATH}")

    mappings = list(imc.get_editor_property("mappings"))
    changed = False
    found_shift_modifier = False

    for index, mapping in enumerate(mappings):
        action = mapping.get_editor_property("action")
        key = mapping.get_editor_property("key")
        current_key_name = key_name(key)
        current_action_name = asset_name(action)
        log(f"[{index}] key={current_key_name} action={current_action_name}")

        is_left_shift = "LeftShift" in current_key_name or "Left Shift" in current_key_name
        if is_left_shift and action == crouch:
            mapping.set_editor_property("action", modifier)
            changed = True
            found_shift_modifier = True
            log(f"  changed LeftShift mapping from crouch to modifier at index {index}")
        elif is_left_shift and action == modifier:
            found_shift_modifier = True

    if changed:
        imc.set_editor_property("mappings", mappings)

    if not found_shift_modifier:
        add_mapping = getattr(imc, "map_key", None)
        if callable(add_mapping):
            left_shift = unreal.Key()
            left_shift.set_editor_property("key_name", "LeftShift")
            add_mapping(modifier, left_shift)
            changed = True
            log("  added LeftShift mapping to modifier")
        else:
            log("  WARNING: modifier LeftShift mapping not found and map_key was unavailable")

    if changed:
        modifier_saved = unreal.EditorAssetLibrary.save_loaded_asset(modifier)
        imc_saved = unreal.EditorAssetLibrary.save_loaded_asset(imc)
        log(f"save results: modifier={modifier_saved} imc_default={imc_saved}")
        if not imc_saved:
            raise RuntimeError("failed to save IMC_Default; close the running editor and rerun this script")
    else:
        log("no changes needed")

    write_output()


try:
    main()
finally:
    write_output()
