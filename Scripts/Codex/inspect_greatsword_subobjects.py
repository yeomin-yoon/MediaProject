import os
import unreal


OUTPUT_PATH = r"D:\UnrealProject\MediaProject\LyraStarterGame\Saved\Codex\inspect_greatsword_subobjects.txt"
LINES = []


def log(message):
    line = f"[GSSubobject] {message}"
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


def main():
    bp = unreal.EditorAssetLibrary.load_asset("/Game/1dev/OS/B_Test_Hero_ShooterMannequin")
    if not bp:
        raise RuntimeError("Failed to load blueprint")

    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    if not subsystem:
        raise RuntimeError("Failed to get SubobjectDataSubsystem")

    handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)
    log(f"handle_count={len(handles)}")

    for handle in handles:
        found_result = subsystem.k2_find_subobject_data_from_handle(handle)
        log(f"  found_result_type={type(found_result)} value={found_result}")
        if isinstance(found_result, tuple):
            ok = found_result[0]
            data = found_result[1] if len(found_result) > 1 else None
        elif isinstance(found_result, unreal.SubobjectData):
            ok = True
            data = found_result
        else:
            ok = bool(found_result)
            data = None

        log(f"handle={handle} ok={ok}")
        if not ok or not data:
            continue

        try:
            display = data.get_display_string(True)
        except Exception as exc:
            display = f"<display_error:{exc}>"
        obj = None
        try:
            obj = data.get_object_for_blueprint(bp)
        except Exception as exc:
            log(f"  get_object_for_blueprint failed: {exc}")
        if not obj:
            try:
                obj = data.get_object()
            except Exception as exc:
                log(f"  get_object failed: {exc}")

        log(f"  display={display}")
        log(f"  object={fmt_asset(obj)}")
        if obj:
            log(f"  class={obj.get_class().get_name()}")
            for prop in [
                "style",
                "weapon_actor_class",
                "attach_socket",
                "relative_attach_transform",
                "input_bindings",
                "trace_source_id",
                "default_trace_profile",
                "static_mesh",
            ]:
                value = try_get(obj, prop)
                if isinstance(value, list):
                    log(f"    {prop}.count={len(value)}")
                    for index, item in enumerate(value[:12]):
                        log(f"      [{index}] {item}")
                else:
                    log(f"    {prop}={value}")

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
        handle.write("\n".join(LINES))


main()
