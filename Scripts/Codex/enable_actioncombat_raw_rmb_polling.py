import os

import unreal


OUTPUT_PATH = r"D:\UnrealProject\MediaProject\LyraStarterGame\Saved\Codex\enable_actioncombat_raw_rmb_polling.txt"
HERO_BP_PATH = "/Game/1dev/OS/DragonKnightRuntime/B_Test_Hero_DragonRuntime"
LINES = []


def log(message):
    LINES.append(message)
    unreal.log("[CodexRawRmb] " + message)


def write_output():
    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
        handle.write("\n".join(LINES))


def find_subobject_object(bp, variable_name):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)
    for handle in handles:
        result = subsystem.k2_find_subobject_data_from_handle(handle)
        data = result[1] if isinstance(result, tuple) and len(result) > 1 else result
        if not data:
            continue
        current_name = unreal.SubobjectDataBlueprintFunctionLibrary.get_variable_name(data)
        if str(current_name) == variable_name:
            return unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, bp)
    return None


def main():
    bp = unreal.EditorAssetLibrary.load_asset(HERO_BP_PATH)
    if not bp:
        raise RuntimeError(f"failed to load {HERO_BP_PATH}")

    bridge = find_subobject_object(bp, "ActionCombatLyraInputBridge")
    if not bridge:
        raise RuntimeError("ActionCombatLyraInputBridge component not found")

    for prop in ("poll_right_mouse_button_for_secondary", "b_poll_right_mouse_button_for_secondary"):
        try:
            old_value = bridge.get_editor_property(prop)
            bridge.set_editor_property(prop, True)
            log(f"set {prop}: {old_value} -> True")
            unreal.BlueprintEditorLibrary.compile_blueprint(bp)
            saved = unreal.EditorAssetLibrary.save_loaded_asset(bp)
            log(f"saved={saved}")
            return
        except Exception as exc:
            log(f"{prop} failed: {exc}")

    raise RuntimeError("raw RMB polling property was not found")


try:
    main()
finally:
    write_output()
