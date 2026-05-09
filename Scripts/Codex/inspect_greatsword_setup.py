import os
import unreal


OUTPUT_PATH = r"D:\UnrealProject\MediaProject\LyraStarterGame\Saved\Codex\inspect_greatsword_setup.txt"
LINES = []


SOURCE_ANIMS = [
    "/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_A/AS_GS_Attack_A_01",
    "/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_A/AS_GS_Attack_A_02",
    "/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_A/AS_GS_Attack_A_03",
    "/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_A/AS_GS_Attack_A_03_B",
]


def log(message):
    line = f"[GSInspect] {message}"
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


def inspect_anim(anim_path):
    anim = unreal.EditorAssetLibrary.load_asset(anim_path)
    if not anim:
        log(f"FAILED load anim {anim_path}")
        return

    skeleton = try_get(anim, "skeleton")
    preview = try_get(anim, "preview_pose_asset")
    retarget = try_get(anim, "retarget_source_asset")
    log(f"anim={anim_path}")
    log(f"  class={anim.get_class().get_name()}")
    log(f"  skeleton={fmt_asset(skeleton)}")
    log(f"  preview_pose_asset={fmt_asset(preview)}")
    log(f"  retarget_source_asset={fmt_asset(retarget)}")


def inspect_blueprint(bp_path):
    asset = unreal.EditorAssetLibrary.load_asset(bp_path)
    if not asset:
        log(f"FAILED load blueprint {bp_path}")
        return

    log(f"blueprint={bp_path} class={asset.get_class().get_name()}")
    scs = try_get(asset, "simple_construction_script")
    log(f"  simple_construction_script={scs}")
    if isinstance(scs, str) or not scs:
        return

    try:
        nodes = scs.get_all_nodes()
    except Exception as exc:
        log(f"  get_all_nodes failed: {exc}")
        return

    log(f"  scs_node_count={len(nodes)}")
    for node in nodes:
        try:
            node_name = node.get_variable_name()
        except Exception:
            node_name = str(node)
        template = try_get(node, "component_template")
        comp_class = template.get_class().get_name() if hasattr(template, "get_class") else "<unknown>"
        log(f"  node={node_name} component_class={comp_class} template={fmt_asset(template)}")

        for prop in [
            "style",
            "weapon_actor_class",
            "attach_socket",
            "relative_attach_transform",
            "action_combat_component_reference",
            "guard_component_reference",
            "input_bindings",
            "trace_source_id",
            "trace_source_component",
            "default_trace_profile",
            "static_mesh",
        ]:
            value = try_get(template, prop)
            if isinstance(value, list):
                log(f"    {prop}.count={len(value)}")
                for index, item in enumerate(value[:12]):
                    log(f"      [{index}] {item}")
            else:
                log(f"    {prop}={value}")


def main():
    target_skeleton = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Heroes/Mannequin/Meshes/SKM_Manny")
    log(f"target_manny_mesh={fmt_asset(target_skeleton)}")
    log(f"target_manny_mesh.skeleton={fmt_asset(try_get(target_skeleton, 'skeleton'))}")

    for anim_path in SOURCE_ANIMS:
        inspect_anim(anim_path)

    inspect_blueprint("/Game/1dev/OS/B_Test_Hero_ShooterMannequin")

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
        handle.write("\n".join(LINES))


main()
