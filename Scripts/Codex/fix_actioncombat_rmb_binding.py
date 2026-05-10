import os

import unreal


OUTPUT_PATH = r"D:\UnrealProject\MediaProject\LyraStarterGame\Saved\Codex\fix_actioncombat_rmb_binding.txt"
HERO_BP_PATH = "/Game/1dev/OS/DragonKnightRuntime/B_Test_Hero_DragonRuntime"
LINES = []


def log(message):
    LINES.append(message)
    unreal.log("[CodexRmbBinding] " + message)


def write_output():
    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
        handle.write("\n".join(LINES))


def make_tag(tag_name):
    tag = unreal.GameplayTag()
    if tag_name:
        tag.import_text('(TagName="{}")'.format(tag_name))
    return tag


def tag_text(tag):
    if not tag:
        return "None"
    for attr in ("to_string", "get_tag_name"):
        fn = getattr(tag, attr, None)
        if callable(fn):
            try:
                value = fn()
                if value:
                    return str(value)
            except Exception:
                pass
    try:
        return str(tag.get_editor_property("tag_name"))
    except Exception:
        return str(tag)


def build_binding(input_tag, started_command="", held_state="", mirror=False, repeat=False, repeat_interval=0.24):
    binding = unreal.ActionCombatLyraInputBinding()
    binding.set_editor_property("input_tag", make_tag(input_tag))
    binding.set_editor_property("started_command_tag", make_tag(started_command))
    binding.set_editor_property("completed_command_tag", make_tag(""))
    binding.set_editor_property("held_input_state_tag", make_tag(held_state))
    binding.set_editor_property("mirror_held_state_while_pressed", mirror)
    binding.set_editor_property("repeat_started_command_while_held", repeat)
    binding.set_editor_property("started_command_repeat_interval_seconds", repeat_interval)
    binding.set_editor_property("set_focus_active_on_started", False)
    binding.set_editor_property("set_focus_inactive_on_completed", False)
    binding.set_editor_property("set_guard_input_held_on_started", False)
    binding.set_editor_property("clear_guard_input_held_on_completed", False)
    return binding


def find_subobject_object(bp, variable_name):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)
    for handle in handles:
        result = subsystem.k2_find_subobject_data_from_handle(handle)
        data = result[1] if isinstance(result, tuple) and len(result) > 1 else result
        if not data:
            continue
        current_name = unreal.SubobjectDataBlueprintFunctionLibrary.get_variable_name(data)
        if str(current_name) != variable_name:
            continue
        return unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, bp)
    return None


def dump_bindings(prefix, bindings):
    for index, binding in enumerate(list(bindings)):
        log(
            f"{prefix}[{index}] input={tag_text(binding.get_editor_property('input_tag'))} "
            f"started={tag_text(binding.get_editor_property('started_command_tag'))} "
            f"held={tag_text(binding.get_editor_property('held_input_state_tag'))} "
            f"mirror={binding.get_editor_property('mirror_held_state_while_pressed')} "
            f"repeat={binding.get_editor_property('repeat_started_command_while_held')} "
            f"interval={binding.get_editor_property('started_command_repeat_interval_seconds')}"
        )


def main():
    bp = unreal.EditorAssetLibrary.load_asset(HERO_BP_PATH)
    if not bp:
        raise RuntimeError(f"failed to load {HERO_BP_PATH}")

    bridge = find_subobject_object(bp, "ActionCombatLyraInputBridge")
    if not bridge:
        raise RuntimeError("ActionCombatLyraInputBridge component not found")

    old_bindings = bridge.get_editor_property("input_bindings")
    dump_bindings("old", old_bindings)

    new_bindings = [
        build_binding("InputTag.Combat.Attack.Primary", "Combat.Command.Light", repeat=True),
        build_binding("InputTag.Combat.Attack.Secondary", "Combat.Command.Alt", repeat=True),
        build_binding("InputTag.Combat.Modifier", "", "Combat.Input.Held.Modifier", mirror=True),
    ]
    bridge.set_editor_property("input_bindings", new_bindings)
    dump_bindings("new", bridge.get_editor_property("input_bindings"))

    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    saved = unreal.EditorAssetLibrary.save_loaded_asset(bp)
    log(f"saved={saved}")
    if not saved:
        raise RuntimeError("failed to save hero blueprint")


try:
    main()
finally:
    write_output()
