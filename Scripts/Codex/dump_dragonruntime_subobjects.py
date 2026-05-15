import os
import unreal


OUTPUT_PATH = r"D:\UnrealProject\MediaProject\LyraStarterGame\Saved\Codex\dump_dragonruntime_subobjects.txt"
LINES = []


def log(message):
    line = f"[DragonSubobjects] {message}"
    LINES.append(line)
    unreal.log(line)


def safe_get(obj, name):
    try:
        return obj.get_editor_property(name)
    except Exception as exc:
        return f"<error:{exc}>"


def fmt(value):
    if value is None:
        return "<none>"
    if hasattr(value, "get_path_name"):
        try:
            return value.get_path_name()
        except Exception:
            pass
    return str(value)


def dump_blueprint_subobjects(asset_path):
    bp = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not bp:
        log(f"FAILED load {asset_path}")
        return

    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)
    log(f"blueprint={asset_path} handle_count={len(handles)}")

    for index, handle in enumerate(handles):
        try:
            found_result = subsystem.k2_find_subobject_data_from_handle(handle)
        except Exception as exc:
            log(f"  handle[{index}] find failed: {exc}")
            continue

        if isinstance(found_result, tuple):
            found = found_result[0]
            data = found_result[1] if len(found_result) > 1 else None
        else:
            found = found_result is not None
            data = found_result

        if not found or not data:
            log(f"  handle[{index}] not found")
            continue

        try:
            variable_name = unreal.SubobjectDataBlueprintFunctionLibrary.get_variable_name(data)
        except Exception as exc:
            variable_name = f"<var_error:{exc}>"

        try:
            obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, bp)
        except Exception as exc:
            obj = None
            log(f"  handle[{index}] get_object_for_blueprint failed: {exc}")

        if not obj:
            try:
                obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
            except Exception as exc:
                log(f"  handle[{index}] get_object failed: {exc}")
                obj = None

        log(f"  handle[{index}] var={variable_name} object={fmt(obj)}")
        if not obj:
            continue

        log(f"    class={obj.get_class().get_name()}")
        for prop in [
            "style",
            "style_layers",
            "weapon_actor_class",
            "attach_socket",
            "relative_attach_transform",
            "attach_parent_component",
            "action_combat_component_reference",
            "guard_component_reference",
            "input_bindings",
            "b_log_binding_flow",
            "b_bind_only_locally_controlled",
            "trace_source_id",
            "trace_source_component",
            "default_trace_profile",
            "static_mesh",
            "skeletal_mesh",
        ]:
            value = safe_get(obj, prop)
            if isinstance(value, list):
                log(f"    {prop}.count={len(value)}")
                for item_index, item in enumerate(value[:16]):
                    log(f"      [{item_index}] {item}")
            else:
                log(f"    {prop}={value}")


def main():
    dump_blueprint_subobjects("/Game/1dev/OS/DragonKnightRuntime/B_Test_Hero_DragonRuntime")
    dump_blueprint_subobjects("/Game/1dev/OS/Weapon/B_MeeleWeapon_Test")

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
        handle.write("\n".join(LINES))


main()
