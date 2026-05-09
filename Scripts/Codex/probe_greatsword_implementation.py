import os
import unreal


OUTPUT_PATH = r"D:\UnrealProject\MediaProject\LyraStarterGame\Saved\Codex\probe_greatsword_implementation.txt"
LINES = []


def log(message):
    line = f"[ProbeGS] {message}"
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


def dump_object_properties(obj, title, interesting_names):
    log(f"{title} class={obj.get_class().get_name()} path={fmt(obj)}")
    for name in interesting_names:
        value = safe_get(obj, name)
        if isinstance(value, list):
            log(f"  {name}.count={len(value)}")
            for index, item in enumerate(value[:24]):
                log(f"    [{index}] {fmt(item)}")
        else:
            log(f"  {name}={fmt(value)}")


def inspect_blueprint_defaults(asset_path, extra_prop_filters):
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not asset:
        log(f"FAILED load asset {asset_path}")
        return

    dump_object_properties(
        asset,
        f"asset={asset_path}",
        ["generated_class", "parent_class", "blueprint_type"],
    )

    generated_class = None
    try:
        generated_class = unreal.EditorAssetLibrary.load_blueprint_class(asset_path)
        log(f"  load_blueprint_class={fmt(generated_class)}")
    except Exception as exc:
        log(f"  load_blueprint_class failed: {exc}")

    if not generated_class:
        try:
            asset_name = asset_path.rsplit("/", 1)[-1]
            object_path = asset_path + "." + asset_name + "_C"
            generated_class = unreal.load_object(None, object_path)
            log(f"  load_object class path={object_path} -> {fmt(generated_class)}")
        except Exception as exc:
            log(f"  load_object generated class failed: {exc}")

    if not generated_class:
        log("  generated_class unresolved")
        return

    cdo = unreal.get_default_object(generated_class)
    dump_object_properties(
        cdo,
        "cdo",
        extra_prop_filters,
    )

    if not hasattr(cdo, "get_components_by_class"):
        log("  cdo has no get_components_by_class")
        return

    components = cdo.get_components_by_class(unreal.ActorComponent)
    log(f"  component_count={len(components)}")
    for comp in components:
        comp_name = comp.get_name()
        comp_class = comp.get_class().get_name()
        log(f"  component {comp_name} class={comp_class}")
        for name in [
            "style",
            "style_layers",
            "weapon_actor_class",
            "attach_socket",
            "relative_attach_transform",
            "action_combat_component_reference",
            "guard_component_reference",
            "input_bindings",
            "held_input_state_tag",
            "trace_source_id",
            "trace_source_component",
            "default_trace_profile",
            "static_mesh",
            "skeletal_mesh",
        ]:
            value = safe_get(comp, name)
            if isinstance(value, list):
                log(f"    {name}.count={len(value)}")
                for index, item in enumerate(value[:24]):
                    log(f"      [{index}] {fmt(item)}")
            else:
                log(f"    {name}={fmt(value)}")


def inspect_mesh(asset_path):
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not asset:
        log(f"FAILED load mesh {asset_path}")
        return

    dump_object_properties(
        asset,
        f"mesh={asset_path}",
        ["skeleton", "physics_asset", "materials", "sockets"],
    )

    if hasattr(asset, "get_all_socket_names"):
        try:
            sockets = asset.get_all_socket_names()
            log(f"  get_all_socket_names.count={len(sockets)}")
            for socket_name in sockets:
                log(f"    socket={socket_name}")
        except Exception as exc:
            log(f"  get_all_socket_names failed: {exc}")


def inspect_unreal_api():
    for class_name in [
        "AnimMontageFactory",
        "BlueprintEditorLibrary",
        "EditorAssetLibrary",
        "SubobjectDataSubsystem",
        "AddNewSubobjectParams",
    ]:
        cls = getattr(unreal, class_name, None)
        log(f"api {class_name} exists={cls is not None}")
        if cls is None:
            continue

        sample = cls
        if class_name == "AnimMontageFactory":
            sample = unreal.AnimMontageFactory()
        elif class_name == "AddNewSubobjectParams":
            sample = unreal.AddNewSubobjectParams()

        names = sorted(name for name in dir(sample) if not name.startswith("_"))
        log(f"  names[{class_name}]={names[:120]}")


def main():
    inspect_unreal_api()

    inspect_blueprint_defaults(
        "/Game/1dev/OS/DragonKnightRuntime/B_Test_Hero_DragonRuntime",
        [
            "weapon_actor_class",
            "style",
            "style_layers",
            "equipment_definitions",
            "pawn_ext_component",
        ],
    )

    inspect_blueprint_defaults(
        "/Game/1dev/OS/DragonKnightRuntime/B_Test_ActionCombat_DragonRuntime_Elimination",
        [
            "default_pawn_data",
            "pawn_data",
            "experience_data",
            "default_game_mode",
            "game_features_to_enable",
            "action_sets",
        ],
    )

    for mesh_path in [
        "/Game/1dev/OS/SimpleGreatSwordAnim/Demo/Weapons/SM_GreatSword",
        "/Game/GreatSword/Mannequin/Character/Mesh/WeaponMaster_GreatSword",
        "/Game/GreatSword/Mannequin/Character/Mesh/WeaponMaster_GreatSword01",
        "/Game/GreatSword/GreatSword/Weapon/GreatSword_00",
        "/Game/DF_DRAGON_KNIGHT/MESHES/SWORD/SK_Dragon_knight_sword",
    ]:
        inspect_mesh(mesh_path)

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
        handle.write("\n".join(LINES))


main()
