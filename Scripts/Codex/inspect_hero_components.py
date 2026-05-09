import os
import unreal


OUTPUT_PATH = r"D:\UnrealProject\MediaProject\LyraStarterGame\Saved\Codex\inspect_hero_components.txt"
LINES = []


def log(message):
    line = "[HeroComponents] {}".format(message)
    LINES.append(line)
    unreal.log(line)


def try_get(obj, prop):
    try:
        return obj.get_editor_property(prop)
    except Exception as exc:
        return "<error:{}>".format(exc)


def main():
    bp = unreal.EditorAssetLibrary.load_asset("/Game/1dev/OS/DragonKnightRuntime/B_Test_Hero_DragonRuntime")
    if not bp:
        raise RuntimeError("Failed to load hero bp")

    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)
    log("handle_count={}".format(len(handles)))
    for handle in handles:
        data = subsystem.k2_find_subobject_data_from_handle(handle)
        if isinstance(data, tuple):
            ok = data[0]
            data = data[1] if len(data) > 1 else None
            if not ok or not data:
                continue
        if not data:
            continue
        obj = None
        try:
            obj = data.get_object_for_blueprint(bp)
        except Exception:
            obj = None
        if not obj:
            try:
                obj = data.get_object()
            except Exception:
                obj = None
        if obj:
            log("subobject={} class={}".format(obj.get_name(), obj.get_class().get_name()))

    generated_class = unreal.BlueprintEditorLibrary.generated_class(bp)
    cdo = unreal.get_default_object(generated_class)
    comps = cdo.get_components_by_class(unreal.ActorComponent)
    log("cdo_component_count={}".format(len(comps)))
    for comp in comps:
        log("component={} class={}".format(comp.get_name(), comp.get_class().get_name()))
        for prop in ["style_layers", "input_bindings", "weapon_actor_class"]:
            value = try_get(comp, prop)
            if not (isinstance(value, str) and value.startswith("<error:")):
                log("  {}={}".format(prop, value))

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
        handle.write("\n".join(LINES))


main()
