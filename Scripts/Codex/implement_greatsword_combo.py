import datetime
import os
import traceback
import unreal


PROJECT_ROOT = r"D:\UnrealProject\MediaProject\LyraStarterGame"
LOG_PATH = os.path.join(PROJECT_ROOT, "Saved", "Codex", "implement_greatsword_combo.txt")

SOURCE_MESH_PATH = "/Game/1dev/OS/SimpleGreatSwordAnim/Demo/Mannequins/Meshes/SKM_Manny"
TARGET_MESH_PATH = "/Game/Characters/Heroes/Mannequin/Meshes/SKM_Manny"
TARGET_IK_RIG_PATH = "/Game/Characters/Heroes/Mannequin/Rig/IK_Mannequin"

RIG_DIR = "/Game/1dev/ActionCombat/GreatSword/Rig"
ANIM_DIR = "/Game/1dev/ActionCombat/GreatSword/Animations/Retargeted/Manny"
MONTAGE_DIR = "/Game/1dev/ActionCombat/GreatSword/Montages"
STYLE_DIR = "/Game/1dev/ActionCombat/GreatSword/Styles"
WEAPON_DIR = "/Game/1dev/ActionCombat/GreatSword/Weapons"
BACKUP_DIR = "/Game/1dev/ActionCombat/GreatSword/Backups"

LIVE_STYLE_PATH = "/Game/1dev/OS/GDHOneHanded/PrimaryAttack_GDHOneHanded_Auto"
GREATSWORD_STYLE_PATH = STYLE_DIR + "/DA_ActionCombatStyle_GreatSword_Test"
LIVE_HERO_BP_PATH = "/Game/1dev/OS/DragonKnightRuntime/B_Test_Hero_DragonRuntime"
LIVE_WEAPON_BP_PATH = "/Game/1dev/OS/Weapon/B_MeeleWeapon_Test"
GREATSWORD_WEAPON_BP_PATH = WEAPON_DIR + "/B_MeleeWeapon_GreatSword_Test"
INPUT_CONFIG_PATH = "/Game/1dev/OS/Test_InputData_Hero"

SOURCE_ANIMS = [
    {
        "source": "/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_A/AS_GS_Attack_A_01",
        "sequence_name": "A_GS_Attack_A_01_Manny",
        "montage_name": "AM_GS_Attack_A_01",
        "action_tag": "Combat.Action.GreatSword.A.01",
        "notify": (0.30, 0.52),
        "advance": (75.0, 0.12, 0.44, 1.20),
        "poise": 35.0,
    },
    {
        "source": "/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_A/AS_GS_Attack_A_02",
        "sequence_name": "A_GS_Attack_A_02_Manny",
        "montage_name": "AM_GS_Attack_A_02",
        "action_tag": "Combat.Action.GreatSword.A.02",
        "notify": (0.28, 0.54),
        "advance": (90.0, 0.10, 0.46, 1.20),
        "poise": 40.0,
    },
    {
        "source": "/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_A/AS_GS_Attack_A_03",
        "sequence_name": "A_GS_Attack_A_03_Manny",
        "montage_name": "AM_GS_Attack_A_03",
        "action_tag": "Combat.Action.GreatSword.A.03",
        "notify": (0.28, 0.56),
        "advance": (100.0, 0.10, 0.48, 1.20),
        "poise": 45.0,
    },
    {
        "source": "/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_A/AS_GS_Attack_A_03_B",
        "sequence_name": "A_GS_Attack_A_04_Manny",
        "montage_name": "AM_GS_Attack_A_04",
        "action_tag": "Combat.Action.GreatSword.A.04",
        "notify": (0.34, 0.62),
        "advance": (110.0, 0.12, 0.52, 1.25),
        "poise": 55.0,
    },
    {
        "source": "/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_B/AS_GS_Attack_B_01",
        "sequence_name": "A_GS_Attack_B_01_Manny",
        "montage_name": "AM_GS_Attack_B_01",
        "action_tag": "Combat.Action.GreatSword.B.01",
        "notify": (0.30, 0.56),
        "advance": (85.0, 0.12, 0.48, 1.20),
        "poise": 38.0,
    },
    {
        "source": "/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_B/AS_GS_Attack_B_02",
        "sequence_name": "A_GS_Attack_B_02_Manny",
        "montage_name": "AM_GS_Attack_B_02",
        "action_tag": "Combat.Action.GreatSword.B.02",
        "notify": (0.30, 0.58),
        "advance": (95.0, 0.12, 0.50, 1.20),
        "poise": 42.0,
    },
    {
        "source": "/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_B/AS_GS_Attack_B_03",
        "sequence_name": "A_GS_Attack_B_03_Manny",
        "montage_name": "AM_GS_Attack_B_03",
        "action_tag": "Combat.Action.GreatSword.B.03",
        "notify": (0.32, 0.60),
        "advance": (110.0, 0.14, 0.54, 1.20),
        "poise": 48.0,
    },
    {
        "source": "/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_B/AS_GS_Attack_B_04",
        "sequence_name": "A_GS_Attack_B_04_Manny",
        "montage_name": "AM_GS_Attack_B_04",
        "action_tag": "Combat.Action.GreatSword.B.04",
        "notify": (0.34, 0.62),
        "advance": (120.0, 0.16, 0.58, 1.25),
        "poise": 60.0,
    },
    {
        "source": "/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_C/AS_GS_Attack_C_01",
        "sequence_name": "A_GS_Attack_C_01_Manny",
        "montage_name": "AM_GS_Attack_C_01",
        "action_tag": "Combat.Action.GreatSword.C.01",
        "notify": (0.30, 0.54),
        "advance": (75.0, 0.12, 0.46, 1.20),
        "poise": 36.0,
    },
    {
        "source": "/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_C/AS_GS_Attack_C_02",
        "sequence_name": "A_GS_Attack_C_02_Manny",
        "montage_name": "AM_GS_Attack_C_02",
        "action_tag": "Combat.Action.GreatSword.C.02",
        "notify": (0.32, 0.58),
        "advance": (90.0, 0.12, 0.50, 1.20),
        "poise": 44.0,
    },
    {
        "source": "/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_C/AS_GS_Attack_C_03",
        "sequence_name": "A_GS_Attack_C_03_Manny",
        "montage_name": "AM_GS_Attack_C_03",
        "action_tag": "Combat.Action.GreatSword.C.03",
        "notify": (0.34, 0.62),
        "advance": (110.0, 0.16, 0.56, 1.25),
        "poise": 58.0,
    },
    {
        "source": "/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_D/AS_GS_Attack_D_01",
        "sequence_name": "A_GS_Attack_D_01_Manny",
        "montage_name": "AM_GS_Attack_D_01",
        "action_tag": "Combat.Action.GreatSword.D.01",
        "notify": (0.34, 0.60),
        "advance": (100.0, 0.16, 0.55, 1.30),
        "poise": 50.0,
    },
    {
        "source": "/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_D/AS_GS_Attack_D_02",
        "sequence_name": "A_GS_Attack_D_02_Manny",
        "montage_name": "AM_GS_Attack_D_02",
        "action_tag": "Combat.Action.GreatSword.D.02",
        "notify": (0.36, 0.62),
        "advance": (120.0, 0.18, 0.60, 1.30),
        "poise": 60.0,
    },
    {
        "source": "/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_D/AS_GS_Attack_D_03",
        "sequence_name": "A_GS_Attack_D_03_Manny",
        "montage_name": "AM_GS_Attack_D_03",
        "action_tag": "Combat.Action.GreatSword.D.03",
        "notify": (0.38, 0.66),
        "advance": (140.0, 0.22, 0.65, 1.35),
        "poise": 70.0,
    },
]

LINES = []


def log(message):
    line = "[GreatSwordCombo] {}".format(message)
    LINES.append(line)
    unreal.log(line)


def log_error(message):
    line = "[GreatSwordCombo] ERROR {}".format(message)
    LINES.append(line)
    unreal.log_error(line)


def ensure_dir(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def asset_path_to_object_path(asset_path):
    name = asset_path.rsplit("/", 1)[-1]
    return "{}.{}".format(asset_path, name)


def load_asset(asset_path):
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not asset:
        raise RuntimeError("Failed to load asset {}".format(asset_path))
    return asset


def delete_asset_if_exists(asset_path):
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.EditorAssetLibrary.delete_asset(asset_path)


def duplicate_asset_overwrite(source_path, target_path):
    delete_asset_if_exists(target_path)
    target_dir = target_path.rsplit("/", 1)[0]
    ensure_dir(target_dir)
    ok = unreal.EditorAssetLibrary.duplicate_asset(source_path, target_path)
    if not ok:
        raise RuntimeError("Failed to duplicate {} -> {}".format(source_path, target_path))
    return load_asset(target_path)


def timestamp_string():
    return datetime.datetime.now().strftime("%Y%m%d_%H%M%S")


def safe_get(obj, prop):
    try:
        return obj.get_editor_property(prop)
    except Exception:
        return None


def make_tag(tag_name):
    if not tag_name:
        return unreal.GameplayTag()
    tag = unreal.GameplayTag()
    tag.import_text('(TagName="{}")'.format(tag_name))
    return tag


def tag_text(tag_value):
    if not tag_value:
        return ""
    exported = tag_value.export_text()
    marker = 'TagName="'
    start = exported.find(marker)
    if start >= 0:
        start += len(marker)
        end = exported.find('"', start)
        if end > start:
            return exported[start:end]
    return exported


def make_tag_container(tag_names):
    container = unreal.GameplayTagContainer()
    filtered = [name for name in tag_names if name]
    if filtered:
        container.import_text('(GameplayTags=({}))'.format(",".join('"{}"'.format(name) for name in filtered)))
    return container


def find_action_spec_by_source_name(source_name):
    for spec in SOURCE_ANIMS:
        if spec["source"].rsplit("/", 1)[-1] == source_name:
            return spec
    return None


def create_asset(asset_name, package_path, asset_class, factory):
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    delete_asset_if_exists("{}/{}".format(package_path, asset_name))
    asset = tools.create_asset(asset_name, package_path, asset_class, factory)
    if not asset:
        raise RuntimeError("Failed to create asset {}/{}".format(package_path, asset_name))
    return asset


def build_source_ik_rig(source_mesh):
    ensure_dir(RIG_DIR)
    rig_path = RIG_DIR + "/IK_SimpleGreatSword_Source"
    delete_asset_if_exists(rig_path)
    rig = create_asset("IK_SimpleGreatSword_Source", RIG_DIR, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    controller = unreal.IKRigController.get_controller(rig)
    controller.set_skeletal_mesh(source_mesh)
    controller.set_retarget_root("pelvis")

    chain_specs = [
        ("Root", "root", "root", ""),
        ("Spine", "spine_01", "spine_03", ""),
        ("Neck", "neck_01", "neck_01", ""),
        ("Head", "Head", "Head", ""),
        ("LeftLeg", "thigh_l", "ball_l", "LeftFootIK"),
        ("LeftClavicle", "clavicle_l", "clavicle_l", ""),
        ("LeftArm", "upperarm_l", "hand_l", "LeftHandIK"),
        ("LeftThumb", "thumb_01_l", "thumb_03_l", ""),
        ("LeftIndex", "index_01_l", "index_03_l", ""),
        ("LeftMiddle", "middle_01_l", "middle_03_l", ""),
        ("LeftRing", "ring_01_l", "ring_03_l", ""),
        ("LeftPinky", "pinky_01_l", "pinky_03_l", ""),
        ("RightLeg", "thigh_r", "ball_r", "RightFootIK"),
        ("RightClavicle", "clavicle_r", "clavicle_r", ""),
        ("RightArm", "upperarm_r", "hand_r", "RightHandIK"),
        ("RightThumb", "thumb_01_r", "thumb_03_r", ""),
        ("RightIndex", "index_01_r", "index_03_r", ""),
        ("RightMiddle", "middle_01_r", "middle_03_r", ""),
        ("RightRing", "ring_01_r", "ring_03_r", ""),
        ("RightPinky", "pinky_01_r", "pinky_03_r", ""),
    ]
    for chain_name, start_bone, end_bone, goal_name in chain_specs:
        controller.add_retarget_chain(chain_name, start_bone, end_bone, goal_name)
    unreal.EditorAssetLibrary.save_loaded_asset(rig)
    return rig


def build_retargeter(source_ik_rig, target_ik_rig, source_mesh, target_mesh):
    ensure_dir(RIG_DIR)
    retargeter_path = RIG_DIR + "/RTG_SimpleGreatSword_To_LyraManny"
    delete_asset_if_exists(retargeter_path)
    retargeter = create_asset("RTG_SimpleGreatSword_To_LyraManny", RIG_DIR, unreal.IKRetargeter, unreal.IKRetargetFactory())
    controller = unreal.IKRetargeterController.get_controller(retargeter)
    controller.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, source_ik_rig)
    controller.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, target_ik_rig)
    controller.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE, source_mesh)
    controller.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET, target_mesh)
    controller.auto_map_chains(unreal.AutoMapChainType.EXACT, True)
    controller.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET)
    unreal.EditorAssetLibrary.save_loaded_asset(retargeter)
    return retargeter


def retarget_sequences():
    ensure_dir(ANIM_DIR)
    final_paths = []
    temp_paths = []
    for spec in SOURCE_ANIMS:
        final_paths.append("{}/{}".format(ANIM_DIR, spec["sequence_name"]))
        temp_paths.append("/Game/TMP_GS_{}".format(spec["source"].rsplit("/", 1)[-1]))

    if all(unreal.EditorAssetLibrary.does_asset_exist(path) for path in final_paths):
        log("Reusing existing retargeted Manny sequences.")
        return {spec["sequence_name"]: load_asset("{}/{}".format(ANIM_DIR, spec["sequence_name"])) for spec in SOURCE_ANIMS}

    if not all(unreal.EditorAssetLibrary.does_asset_exist(path) for path in temp_paths):
        source_mesh = load_asset(SOURCE_MESH_PATH)
        target_mesh = load_asset(TARGET_MESH_PATH)
        target_ik_rig = load_asset(TARGET_IK_RIG_PATH)
        source_ik_rig = build_source_ik_rig(source_mesh)
        retargeter = build_retargeter(source_ik_rig, target_ik_rig, source_mesh, target_mesh)

        for temp_path in temp_paths:
            delete_asset_if_exists(temp_path)

        source_anims = [unreal.EditorAssetLibrary.find_asset_data(spec["source"]) for spec in SOURCE_ANIMS]
        try:
            unreal.IKRetargetBatchOperation.duplicate_and_retarget(
                source_anims,
                source_mesh,
                target_mesh,
                retargeter,
                "",
                "",
                "TMP_GS_",
                "",
                False,
            )
        except Exception as exc:
            log("Retarget return conversion fallback: {}".format(exc))
    else:
        log("Reusing existing TMP_GS retarget outputs.")

    moved = {}
    for spec in SOURCE_ANIMS:
        source_name = spec["source"].rsplit("/", 1)[-1]
        current_asset_path = "/Game/TMP_GS_{}".format(source_name)
        if not unreal.EditorAssetLibrary.does_asset_exist(current_asset_path):
            raise RuntimeError("Expected retarget output missing: {}".format(current_asset_path))
        target_asset_path = "{}/{}".format(ANIM_DIR, spec["sequence_name"])
        delete_asset_if_exists(target_asset_path)
        ok = unreal.EditorAssetLibrary.rename_asset(current_asset_path, target_asset_path)
        if not ok:
            raise RuntimeError("Failed to move {} -> {}".format(current_asset_path, target_asset_path))
        moved[spec["sequence_name"]] = load_asset(target_asset_path)
        log("Retargeted {} -> {}".format(source_name, target_asset_path))
    return moved


def add_hit_notify_to_sequence(sequence, start_normalized, end_normalized):
    try:
        unreal.AnimationLibrary.add_animation_notify_track(sequence, "ActionCombat")
    except Exception:
        pass

    sequence_length = 0.0
    if hasattr(sequence, "get_play_length"):
        sequence_length = sequence.get_play_length()
    if sequence_length <= 0.0:
        sequence_length = safe_get(sequence, "sequence_length") or 1.0

    start_time = sequence_length * start_normalized
    duration = max(sequence_length * (end_normalized - start_normalized), 0.01)
    notify_state = unreal.AnimationLibrary.add_animation_notify_state_event(
        sequence,
        "ActionCombat",
        start_time,
        duration,
        unreal.AnimNotifyState_ActionCombatHitWindow,
    )
    if notify_state:
        notify_state.set_editor_property("trace_source_id", "WeaponBlade")
        notify_state.set_editor_property("window_name", "BladeActive")
        notify_state.set_editor_property("use_override_profile", False)
    unreal.EditorAssetLibrary.save_loaded_asset(sequence)


def create_montage_from_sequence(sequence, montage_name, target_skeleton):
    ensure_dir(MONTAGE_DIR)
    montage_path = "{}/{}".format(MONTAGE_DIR, montage_name)
    delete_asset_if_exists(montage_path)
    factory = unreal.AnimMontageFactory()
    factory.set_editor_property("source_animation", sequence)
    montage = create_asset(montage_name, MONTAGE_DIR, unreal.AnimMontage, factory)
    unreal.EditorAssetLibrary.save_loaded_asset(montage)
    try:
        slot_names = unreal.AnimationLibrary.get_montage_slot_names(montage)
        log("Created montage {} slots={}".format(montage_name, list(slot_names)))
    except Exception as exc:
        log("Montage slot query failed for {}: {}".format(montage_name, exc))
    return montage


def backup_live_assets():
    ensure_dir(BACKUP_DIR)
    stamp = timestamp_string()
    duplicate_asset_overwrite(LIVE_STYLE_PATH, "{}/PrimaryAttack_GDHOneHanded_Auto_Backup_{}".format(BACKUP_DIR, stamp))
    duplicate_asset_overwrite(INPUT_CONFIG_PATH, "{}/Test_InputData_Hero_Backup_{}".format(BACKUP_DIR, stamp))
    duplicate_asset_overwrite(LIVE_HERO_BP_PATH, "{}/B_Test_Hero_DragonRuntime_Backup_{}".format(BACKUP_DIR, stamp))
    log("Created backups with stamp {}".format(stamp))


def copy_resource_costs_from_live_style():
    live_style = load_asset(LIVE_STYLE_PATH)
    actions = list(live_style.get_editor_property("actions"))
    if not actions:
        return []
    return list(actions[0].get_editor_property("resource_costs"))


def create_action_definition(spec, montage, resource_costs):
    action = unreal.ActionCombatActionDefinition()
    action.set_editor_property("action_tag", make_tag(spec["action_tag"]))
    action.set_editor_property("montage", montage)
    action.set_editor_property("fallback_duration_seconds", 1.0)
    action.set_editor_property("base_play_rate", 1.0)
    action.set_editor_property("queue_window_starts_at_normalized_time", 0.25)
    action.set_editor_property("queue_window_closes_at_normalized_time", 0.85)
    action.set_editor_property("chain_commit_at_normalized_time", 0.70)

    advance = unreal.ActionCombatAttackAdvanceSettings()
    distance, start_time, end_time, curve = spec["advance"]
    advance.set_editor_property("enabled", True)
    advance.set_editor_property("distance", distance)
    advance.set_editor_property("start_normalized_time", start_time)
    advance.set_editor_property("end_normalized_time", end_time)
    advance.set_editor_property("curve_exponent", curve)
    advance.set_editor_property("require_grounded", True)
    advance.set_editor_property("stop_on_blocking_hit", True)
    action.set_editor_property("attack_advance", advance)

    action.set_editor_property("allow_dodge_cancel", True)
    action.set_editor_property("dodge_cancel_starts_at_normalized_time", 0.35)
    action.set_editor_property("trace_source_id", "WeaponBlade")
    action.set_editor_property("hit_window_name", "BladeActive")
    action.set_editor_property("motion_value", 1.0)
    action.set_editor_property("poise_damage", spec["poise"])
    action.set_editor_property("buildup_multiplier", 1.0)
    action.set_editor_property("resource_costs", resource_costs)
    return action


def create_transition(from_tag, command_tag, to_tag, required_tags=None, blocked_tags=None):
    transition = unreal.ActionCombatTransitionDefinition()
    transition.set_editor_property("from_action_tag", make_tag(from_tag))
    transition.set_editor_property("command_tag", make_tag(command_tag))
    transition.set_editor_property("to_action_tag", make_tag(to_tag))
    transition.set_editor_property("requires_focus_active", False)
    transition.set_editor_property("requires_focus_inactive", False)
    transition.set_editor_property("required_held_input_tags", make_tag_container(required_tags or []))
    transition.set_editor_property("blocked_held_input_tags", make_tag_container(blocked_tags or []))
    return transition


def configure_style_asset(style_asset_path, montage_map):
    style = load_asset(style_asset_path)
    resource_costs = copy_resource_costs_from_live_style()
    actions = []
    for spec in SOURCE_ANIMS:
        montage = montage_map[spec["montage_name"]]
        actions.append(create_action_definition(spec, montage, resource_costs))

    modifier_tag = "Combat.Input.Held.Modifier"
    transitions = [
        create_transition("", "Combat.Command.Light", "Combat.Action.GreatSword.C.01", [modifier_tag], []),
        create_transition("Combat.Action.GreatSword.A.01", "Combat.Command.Light", "Combat.Action.GreatSword.C.01", [modifier_tag], []),
        create_transition("Combat.Action.GreatSword.A.02", "Combat.Command.Light", "Combat.Action.GreatSword.C.01", [modifier_tag], []),
        create_transition("Combat.Action.GreatSword.A.03", "Combat.Command.Light", "Combat.Action.GreatSword.C.01", [modifier_tag], []),
        create_transition("", "Combat.Command.Alt", "Combat.Action.GreatSword.B.01"),
        create_transition("Combat.Action.GreatSword.A.01", "Combat.Command.Alt", "Combat.Action.GreatSword.B.01"),
        create_transition("Combat.Action.GreatSword.A.02", "Combat.Command.Alt", "Combat.Action.GreatSword.B.01"),
        create_transition("Combat.Action.GreatSword.A.03", "Combat.Command.Alt", "Combat.Action.GreatSword.B.01"),
        create_transition("Combat.Action.GreatSword.A.04", "Combat.Command.Alt", "Combat.Action.GreatSword.B.01"),
        create_transition("Combat.Action.GreatSword.B.02", "Combat.Command.Alt", "Combat.Action.GreatSword.D.01"),
        create_transition("Combat.Action.GreatSword.C.02", "Combat.Command.Alt", "Combat.Action.GreatSword.D.01"),
        create_transition("", "Combat.Command.Light", "Combat.Action.GreatSword.A.01", [], [modifier_tag]),
        create_transition("Combat.Action.GreatSword.A.01", "Combat.Command.Light", "Combat.Action.GreatSword.A.02", [], [modifier_tag]),
        create_transition("Combat.Action.GreatSword.A.02", "Combat.Command.Light", "Combat.Action.GreatSword.A.03", [], [modifier_tag]),
        create_transition("Combat.Action.GreatSword.A.03", "Combat.Command.Light", "Combat.Action.GreatSword.A.04", [], [modifier_tag]),
        create_transition("Combat.Action.GreatSword.B.01", "Combat.Command.Light", "Combat.Action.GreatSword.B.02"),
        create_transition("Combat.Action.GreatSword.B.02", "Combat.Command.Light", "Combat.Action.GreatSword.B.03"),
        create_transition("Combat.Action.GreatSword.B.03", "Combat.Command.Light", "Combat.Action.GreatSword.B.04"),
        create_transition("Combat.Action.GreatSword.C.01", "Combat.Command.Light", "Combat.Action.GreatSword.C.02"),
        create_transition("Combat.Action.GreatSword.C.02", "Combat.Command.Light", "Combat.Action.GreatSword.C.03"),
        create_transition("Combat.Action.GreatSword.D.01", "Combat.Command.Light", "Combat.Action.GreatSword.D.02"),
        create_transition("Combat.Action.GreatSword.D.02", "Combat.Command.Light", "Combat.Action.GreatSword.D.03"),
    ]

    style.set_editor_property("actions", actions)
    style.set_editor_property("transitions", transitions)
    unreal.EditorAssetLibrary.save_loaded_asset(style)
    log("Configured style {}".format(style_asset_path))


def find_subobject_object(blueprint_asset, variable_name):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = subsystem.k2_gather_subobject_data_for_blueprint(blueprint_asset)
    prefix_match = None
    for handle in handles:
        try:
            found_result = subsystem.k2_find_subobject_data_from_handle(handle)
        except Exception:
            continue
        if isinstance(found_result, tuple):
            found = found_result[0]
            data = found_result[1] if len(found_result) > 1 else None
        else:
            found = found_result is not None
            data = found_result
        if not found or not data:
            continue

        try:
            subobject_var_name = unreal.SubobjectDataBlueprintFunctionLibrary.get_variable_name(data)
        except Exception:
            subobject_var_name = None

        try:
            obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, blueprint_asset)
        except Exception:
            obj = None
        if not obj:
            try:
                obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
            except Exception:
                obj = None
        if not obj:
            continue
        obj_name = obj.get_name()
        subobject_var_name_text = str(subobject_var_name) if subobject_var_name is not None else ""
        exact_names = {
            variable_name,
            "{}_GEN_VARIABLE".format(variable_name),
        }
        if obj_name in exact_names or subobject_var_name_text == variable_name:
            log("Found subobject {} -> {}".format(variable_name, obj_name))
            return obj

        if not prefix_match and (
            obj_name.startswith(variable_name)
            or subobject_var_name_text.startswith(variable_name)
        ):
            prefix_match = obj

    if prefix_match:
        log("Found prefix subobject {} -> {}".format(variable_name, prefix_match.get_name()))
        return prefix_match

    try:
        generated_class = unreal.BlueprintEditorLibrary.generated_class(blueprint_asset)
        cdo = unreal.get_default_object(generated_class)
    except Exception:
        cdo = None

    if cdo:
        try:
            root_component = cdo.get_editor_property("root_component")
        except Exception:
            root_component = None
        if root_component and (root_component.get_name().startswith(variable_name) or root_component.get_name() == variable_name):
            return root_component

        try:
            components = cdo.get_components_by_class(unreal.ActorComponent)
        except Exception:
            components = []
        for component in components:
            if component.get_name().startswith(variable_name) or component.get_name() == variable_name:
                return component
    raise RuntimeError("Failed to find subobject {}".format(variable_name))


def build_style_layer_entry(style_asset):
    entry = unreal.ActionCombatStyleLayerEntry()
    entry.set_editor_property("layer_id", "BaseStyle")
    entry.set_editor_property("priority", 0)
    entry.set_editor_property("style", style_asset)
    entry.set_editor_property("layer_play_rate_multiplier", 1.0)
    entry.set_editor_property("enabled", True)
    return entry


def build_input_binding(input_tag, started_command="", held_state="", mirror=False):
    binding = unreal.ActionCombatLyraInputBinding()
    binding.set_editor_property("input_tag", make_tag(input_tag))
    binding.set_editor_property("started_command_tag", make_tag(started_command))
    binding.set_editor_property("completed_command_tag", make_tag(""))
    binding.set_editor_property("held_input_state_tag", make_tag(held_state))
    binding.set_editor_property("mirror_held_state_while_pressed", mirror)
    binding.set_editor_property("set_focus_active_on_started", False)
    binding.set_editor_property("set_focus_inactive_on_completed", False)
    binding.set_editor_property("set_guard_input_held_on_started", False)
    binding.set_editor_property("clear_guard_input_held_on_completed", False)
    return binding


def ensure_input_mapping(input_config, input_action_path, input_tag_name):
    native_actions = list(input_config.get_editor_property("native_input_actions"))
    filtered = [entry for entry in native_actions if tag_text(entry.get_editor_property("input_tag")) != input_tag_name]
    action_asset = load_asset(input_action_path)
    new_entry = unreal.LyraInputAction()
    new_entry.import_text(
        '(InputAction="/Script/EnhancedInput.InputAction\'{}\'",InputTag=(TagName="{}"))'.format(
            action_asset.get_path_name(),
            input_tag_name,
        )
    )
    filtered.append(new_entry)
    input_config.set_editor_property("native_input_actions", filtered)


def configure_input_config():
    input_config = load_asset(INPUT_CONFIG_PATH)
    ensure_input_mapping(input_config, "/Game/1dev/OS/IA_TestHero_Combat_Primary", "InputTag.Combat.Attack.Primary")
    ensure_input_mapping(input_config, "/Game/Input/Actions/IA_Weapon_Fire_Auto", "InputTag.Combat.Attack.Secondary")
    ensure_input_mapping(input_config, "/Game/Input/Actions/IA_Crouch", "InputTag.Combat.Modifier")
    unreal.EditorAssetLibrary.save_loaded_asset(input_config)
    log("Configured input config {}".format(INPUT_CONFIG_PATH))


def configure_live_hero(style_asset):
    hero_bp = load_asset(LIVE_HERO_BP_PATH)
    action_combat = find_subobject_object(hero_bp, "ActionCombat")
    input_bridge = find_subobject_object(hero_bp, "ActionCombatLyraInputBridge")
    weapon_spawner = find_subobject_object(hero_bp, "ActionCombatLyraMeleeWeaponSpawner")

    action_combat.set_editor_property("style_layers", [build_style_layer_entry(style_asset)])
    animation_mesh_ref = action_combat.get_editor_property("animation_mesh_component")
    animation_mesh_ref.set_editor_property("component_property", "Mesh")
    action_combat.set_editor_property("animation_mesh_component", animation_mesh_ref)
    input_bridge.set_editor_property(
        "input_bindings",
        [
            build_input_binding("InputTag.Combat.Attack.Primary", "Combat.Command.Light"),
            build_input_binding("InputTag.Combat.Attack.Secondary", "Combat.Command.Alt"),
            build_input_binding("InputTag.Combat.Modifier", "", "Combat.Input.Held.Modifier", True),
        ],
    )
    weapon_spawner.set_editor_property("weapon_actor_class", unreal.EditorAssetLibrary.load_blueprint_class(GREATSWORD_WEAPON_BP_PATH))
    unreal.BlueprintEditorLibrary.compile_blueprint(hero_bp)
    unreal.EditorAssetLibrary.save_loaded_asset(hero_bp)
    log("Configured live hero {}".format(LIVE_HERO_BP_PATH))


def make_trace_points():
    offsets = [
        unreal.Vector(15.0, 0.0, 0.0),
        unreal.Vector(55.0, 0.0, 0.0),
        unreal.Vector(95.0, 0.0, 0.0),
        unreal.Vector(135.0, 0.0, 0.0),
    ]
    points = []
    for offset in offsets:
        point = unreal.ActionCombatTracePoint()
        point.set_editor_property("socket_name", "")
        point.set_editor_property("local_offset", offset)
        points.append(point)
    return points


def try_configure_static_weapon_component(weapon_bp):
    static_mesh = None
    for candidate in [
        "/Game/1dev/OS/SimpleGreatSwordAnim/Demo/Weapons/SM_GreatSword",
        "/Game/GreatSword/GreatSword/Weapon/GreatSword_00",
    ]:
        asset = unreal.EditorAssetLibrary.load_asset(candidate)
        if asset and asset.get_class().get_name() == "StaticMesh":
            static_mesh = asset
            break
    if not static_mesh:
        return None

    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = subsystem.k2_gather_subobject_data_for_blueprint(weapon_bp)
    parent_handle = None
    for handle in handles:
        data = subsystem.k2_find_subobject_data_from_handle(handle)
        if isinstance(data, tuple):
            ok = data[0]
            data = data[1] if len(data) > 1 else None
            if not ok or not data:
                continue
        obj = None
        try:
            obj = data.get_object_for_blueprint(weapon_bp)
        except Exception:
            obj = None
        if obj and obj.get_name().startswith("SceneRoot"):
            parent_handle = handle
            break

    if not parent_handle:
        return None

    params = unreal.AddNewSubobjectParams()
    params.set_editor_property("parent_handle", parent_handle)
    params.set_editor_property("new_class", unreal.StaticMeshComponent)
    params.set_editor_property("blueprint_context", weapon_bp)
    params.set_editor_property("conform_transform_to_parent", True)

    result = subsystem.add_new_subobject(params)
    new_handle = None
    if isinstance(result, tuple):
        if len(result) >= 2 and result[0]:
            new_handle = result[1]
    else:
        new_handle = result
    if not new_handle:
        return None

    try:
        subsystem.rename_subobject(new_handle, unreal.Text("GreatSwordVisual"))
    except Exception:
        pass

    data = subsystem.k2_find_subobject_data_from_handle(new_handle)
    if isinstance(data, tuple):
        data = data[1] if len(data) > 1 else None
    if not data:
        return None
    new_component = data.get_object_for_blueprint(weapon_bp)
    if not new_component:
        return None
    new_component.set_editor_property("static_mesh", static_mesh)
    new_component.set_editor_property("visible", True)
    return "GreatSwordVisual"


def configure_weapon_blueprint():
    duplicate_asset_overwrite(LIVE_WEAPON_BP_PATH, GREATSWORD_WEAPON_BP_PATH)
    weapon_bp = load_asset(GREATSWORD_WEAPON_BP_PATH)
    weapon_mesh = find_subobject_object(weapon_bp, "WeaponMesh")
    trace_component = find_subobject_object(weapon_bp, "MeleeTraceComponent")

    trace_component_name = "WeaponMesh"
    visual_assigned = False
    for candidate in [
        "/Game/GreatSword/GreatSword/Weapon/GreatSword_00",
        "/Game/1dev/OS/SimpleGreatSwordAnim/Demo/Weapons/SM_GreatSword",
    ]:
        asset = unreal.EditorAssetLibrary.load_asset(candidate)
        if not asset:
            continue
        asset_class_name = asset.get_class().get_name()
        if asset_class_name == "SkeletalMesh":
            weapon_mesh.set_editor_property("skeletal_mesh", asset)
            visual_assigned = True
            trace_component_name = "WeaponMesh"
            log("Using skeletal GreatSword mesh {}".format(candidate))
            break
        if asset_class_name == "StaticMesh":
            try:
                visual_component_name = try_configure_static_weapon_component(weapon_bp)
            except Exception as exc:
                log("Static mesh component creation failed: {}".format(exc))
                visual_component_name = None
            if visual_component_name:
                weapon_mesh.set_editor_property("skeletal_mesh", None)
                trace_component_name = visual_component_name
                visual_assigned = True
                log("Using static GreatSword mesh {}".format(candidate))
                break

    if not visual_assigned:
        log("GreatSword visual mesh fallback: keeping existing weapon mesh because no compatible override was configured.")

    ref = trace_component.get_editor_property("trace_source_component")
    ref.set_editor_property("component_property", trace_component_name)
    trace_component.set_editor_property("trace_source_component", ref)
    trace_component.set_editor_property("trace_source_id", "WeaponBlade")

    profile = trace_component.get_editor_property("default_trace_profile")
    profile.set_editor_property("trace_points", make_trace_points())
    profile.set_editor_property("sweep_radius", 10.0)
    profile.set_editor_property("max_hit_results_per_tick", 16)
    profile.set_editor_property("max_unique_targets_per_window", 8)
    trace_component.set_editor_property("default_trace_profile", profile)

    unreal.BlueprintEditorLibrary.compile_blueprint(weapon_bp)
    unreal.EditorAssetLibrary.save_loaded_asset(weapon_bp)
    log("Configured weapon {}".format(GREATSWORD_WEAPON_BP_PATH))


def write_log_file():
    os.makedirs(os.path.dirname(LOG_PATH), exist_ok=True)
    with open(LOG_PATH, "w", encoding="utf-8") as handle:
        handle.write("\n".join(LINES))


def main():
    for directory in [RIG_DIR, ANIM_DIR, MONTAGE_DIR, STYLE_DIR, WEAPON_DIR, BACKUP_DIR]:
        ensure_dir(directory)

    backup_live_assets()
    configure_input_config()

    retargeted_sequences = retarget_sequences()
    target_mesh = load_asset(TARGET_MESH_PATH)
    target_skeleton = target_mesh.get_editor_property("skeleton")

    montage_map = {}
    for spec in SOURCE_ANIMS:
        sequence = retargeted_sequences[spec["sequence_name"]]
        add_hit_notify_to_sequence(sequence, spec["notify"][0], spec["notify"][1])
        montage = create_montage_from_sequence(sequence, spec["montage_name"], target_skeleton)
        montage_map[spec["montage_name"]] = montage

    duplicate_asset_overwrite(LIVE_STYLE_PATH, GREATSWORD_STYLE_PATH)
    configure_style_asset(GREATSWORD_STYLE_PATH, montage_map)
    configure_style_asset(LIVE_STYLE_PATH, montage_map)

    configure_weapon_blueprint()
    configure_live_hero(load_asset(GREATSWORD_STYLE_PATH))

    log("SUCCESS GreatSword combo assets created and live test hero rewired.")


try:
    main()
except Exception as exc:
    log_error(str(exc))
    log_error(traceback.format_exc())
    raise
finally:
    write_log_file()
    unreal.SystemLibrary.quit_editor()
