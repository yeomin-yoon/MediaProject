import os

import unreal


OUTPUT_PATH = r"D:\UnrealProject\MediaProject\LyraStarterGame\Saved\Codex\inspect_actioncombat_input_bindings.txt"
LINES = []


def log(message):
    LINES.append(message)
    unreal.log("[CodexInputInspect] " + message)


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


def tag_text(tag):
    if not tag:
        return "<none>"

    for attr in ("to_string", "get_tag_name"):
        fn = getattr(tag, attr, None)
        if callable(fn):
            try:
                value = fn()
                if value:
                    return str(value)
            except Exception:
                pass

    getter = getattr(tag, "get_editor_property", None)
    if callable(getter):
        for prop in ("tag_name", "TagName"):
            try:
                value = getter(prop)
                if value:
                    return str(value)
            except Exception:
                pass

    return str(tag)


def safe_get(obj, prop):
    try:
        return obj.get_editor_property(prop)
    except Exception as exc:
        return f"<error:{exc}>"


def as_iterable(value):
    try:
        len(value)
        return list(value)
    except Exception:
        return None


def inspect_input_config():
    config = unreal.EditorAssetLibrary.load_asset("/Game/1dev/OS/Test_InputData_Hero")
    log(f"input_config={path_text(config)}")
    if not config:
        return

    for prop in ("native_input_actions", "ability_input_actions"):
        actions = safe_get(config, prop)
        actions_list = as_iterable(actions)
        if actions_list is None:
            log(f"{prop}=<not list> {actions}")
            continue

        log(f"{prop}.count={len(actions_list)}")
        for index, item in enumerate(actions_list):
            input_action = safe_get(item, "input_action")
            input_tag = safe_get(item, "input_tag")
            log(f"  [{index}] action={path_text(input_action)} tag={tag_text(input_tag)}")


def inspect_input_bridge():
    bp = unreal.EditorAssetLibrary.load_asset("/Game/1dev/OS/DragonKnightRuntime/B_Test_Hero_DragonRuntime")
    log(f"blueprint={path_text(bp)}")
    if not bp:
        return

    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)
    for handle in handles:
        result = subsystem.k2_find_subobject_data_from_handle(handle)
        data = result[1] if isinstance(result, tuple) and len(result) > 1 else result
        if not data:
            continue

        variable_name = unreal.SubobjectDataBlueprintFunctionLibrary.get_variable_name(data)
        if str(variable_name) != "ActionCombatLyraInputBridge":
            continue

        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, bp)
        log(f"bridge={path_text(obj)}")
        bindings = safe_get(obj, "input_bindings")
        bindings_list = as_iterable(bindings)
        if bindings_list is None:
            log(f"input_bindings=<not list> {bindings}")
            return

        log(f"input_bindings.count={len(bindings_list)}")
        for index, binding in enumerate(bindings_list):
            input_tag = safe_get(binding, "input_tag")
            started = safe_get(binding, "started_command_tag")
            completed = safe_get(binding, "completed_command_tag")
            held = safe_get(binding, "held_input_state_tag")
            mirror = safe_get(binding, "mirror_held_state_while_pressed")
            repeat = safe_get(binding, "repeat_started_command_while_held")
            interval = safe_get(binding, "started_command_repeat_interval_seconds")
            log(
                f"  [{index}] input={tag_text(input_tag)} started={tag_text(started)} "
                f"completed={tag_text(completed)} held={tag_text(held)} "
                f"mirror={mirror} repeat={repeat} repeat_interval={interval}"
            )


try:
    inspect_input_config()
    inspect_input_bridge()
finally:
    write_output()
