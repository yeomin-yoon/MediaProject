import os
import traceback

import unreal


HERO_BP_PATH = "/Game/1dev/OS/DragonKnightRuntime/B_Test_Hero_DragonRuntime"
OUTPUT_PATH = r"D:\UnrealProject\MediaProject\LyraStarterGame\Saved\Codex\enable_lobby_cosmetic_bridge.txt"
LINES = []


def log(message):
    LINES.append(message)
    unreal.log("[CodexLobbyCosmetics] " + message)


def write_output():
    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
        handle.write("\n".join(LINES))


def get_subobject_data(subsystem, handle):
    result = subsystem.k2_find_subobject_data_from_handle(handle)
    if isinstance(result, tuple):
        ok = result[0]
        data = result[1] if len(result) > 1 else None
        return data if ok else None
    return result


def get_object_for_blueprint(data, bp):
    if not data:
        return None
    try:
        return unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, bp)
    except Exception:
        pass
    try:
        return unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
    except Exception:
        return None


def get_variable_name(data):
    try:
        return str(unreal.SubobjectDataBlueprintFunctionLibrary.get_variable_name(data))
    except Exception:
        return ""


def find_subobject(bp, variable_name=None, class_name=None):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(bp):
        data = get_subobject_data(subsystem, handle)
        obj = get_object_for_blueprint(data, bp)
        if not obj:
            continue
        current_var = get_variable_name(data)
        current_class = obj.get_class().get_name()
        if variable_name and (current_var == variable_name or obj.get_name().startswith(variable_name)):
            return handle, obj
        if class_name and current_class == class_name:
            return handle, obj
    return None, None


def get_actor_handle(bp):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)
    if not handles:
        raise RuntimeError("blueprint has no subobject handles")
    return handles[0]


def set_bool_property(obj, names, value):
    for name in names:
        try:
            old_value = obj.get_editor_property(name)
            obj.set_editor_property(name, value)
            log(f"set {obj.get_name()}.{name}: {old_value} -> {value}")
            return True
        except Exception:
            pass
    log(f"could not set any bool property {names} on {obj.get_name()}")
    return False


def configure_component(component):
    set_bool_property(component, ["apply_on_begin_play", "b_apply_on_begin_play"], True)
    set_bool_property(component, ["clear_existing_accessories_before_apply", "b_clear_existing_accessories_before_apply"], True)
    set_bool_property(component, ["log_flow", "b_log_flow"], True)

    for prop_name in ["max_apply_attempts"]:
        try:
            component.set_editor_property(prop_name, 120)
            log(f"set {component.get_name()}.{prop_name}=120")
        except Exception as exc:
            log(f"{prop_name} failed: {exc}")

    for prop_name in ["retry_interval_seconds"]:
        try:
            component.set_editor_property(prop_name, 0.1)
            log(f"set {component.get_name()}.{prop_name}=0.1")
        except Exception as exc:
            log(f"{prop_name} failed: {exc}")

    try:
        ref = component.get_editor_property("accessory_component_reference")
        ref.set_editor_property("component_property", "ActionCombatAccessory")
        component.set_editor_property("accessory_component_reference", ref)
        log("set accessory_component_reference=ActionCombatAccessory")
    except Exception as exc:
        log(f"accessory reference setup skipped: {exc}")


def add_bridge_component(bp):
    _, existing = find_subobject(bp, variable_name="ActionCombatLobbyCosmeticBridge")
    if existing:
        log(f"existing bridge component found: {existing.get_name()}")
        configure_component(existing)
        return existing

    _, existing_by_class = find_subobject(bp, class_name="ActionCombatLobbyCosmeticBridgeComponent")
    if existing_by_class:
        log(f"existing bridge component class found: {existing_by_class.get_name()}")
        configure_component(existing_by_class)
        return existing_by_class

    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    params = unreal.AddNewSubobjectParams()
    params.set_editor_property("parent_handle", get_actor_handle(bp))
    params.set_editor_property("new_class", unreal.ActionCombatLobbyCosmeticBridgeComponent)
    params.set_editor_property("blueprint_context", bp)

    result = subsystem.add_new_subobject(params)
    log(f"add_new_subobject result={result}")

    new_handle, component = find_subobject(bp, class_name="ActionCombatLobbyCosmeticBridgeComponent")
    if not component:
        raise RuntimeError(f"failed to resolve newly added lobby cosmetic bridge component after add_new_subobject: {result}")

    try:
        subsystem.rename_subobject(new_handle, unreal.Text("ActionCombatLobbyCosmeticBridge"))
    except Exception as exc:
        log(f"rename_subobject skipped: {exc}")

    _, renamed_component = find_subobject(bp, variable_name="ActionCombatLobbyCosmeticBridge")
    if renamed_component:
        component = renamed_component
    if not component:
        raise RuntimeError("failed to resolve newly added lobby cosmetic bridge component")

    log(f"added bridge component: {component.get_name()}")
    configure_component(component)
    return component


def main():
    bp = unreal.EditorAssetLibrary.load_asset(HERO_BP_PATH)
    if not bp:
        raise RuntimeError(f"failed to load {HERO_BP_PATH}")

    add_bridge_component(bp)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    saved = unreal.EditorAssetLibrary.save_loaded_asset(bp)
    log(f"saved={saved}")


try:
    main()
except Exception as exc:
    log("ERROR: " + str(exc))
    log(traceback.format_exc())
    raise
finally:
    write_output()
