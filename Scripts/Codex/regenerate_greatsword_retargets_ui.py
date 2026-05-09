import os
import unreal


PROJECT_ROOT = r"D:\UnrealProject\MediaProject\LyraStarterGame"
OUTPUT_LOG = os.path.join(PROJECT_ROOT, "Saved", "Codex", "regenerate_greatsword_retargets_ui.txt")

SOURCE_MESH_PATH = "/Game/1dev/OS/SimpleGreatSwordAnim/Demo/Mannequins/Meshes/SKM_Manny"
TARGET_MESH_PATH = "/Game/Characters/Heroes/Mannequin/Meshes/SKM_Manny"
RETARGETER_PATH = "/Game/1dev/ActionCombat/GreatSword/Rig/RTG_SimpleGreatSword_To_LyraManny"
TARGET_SKELETON_PATH = "/Game/Characters/Heroes/Mannequin/Meshes/SK_Mannequin"
TARGET_SKELETON_OBJECT_PATH = TARGET_SKELETON_PATH + ".SK_Mannequin"
ANIM_DIR = "/Game/1dev/ActionCombat/GreatSword/Animations/Retargeted/Manny"
TEMP_PREFIX = "GSFIX_"

SOURCE_ANIMS = [
    ("/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_A/AS_GS_Attack_A_01", "A_GS_Attack_A_01_Manny"),
    ("/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_A/AS_GS_Attack_A_02", "A_GS_Attack_A_02_Manny"),
    ("/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_A/AS_GS_Attack_A_03", "A_GS_Attack_A_03_Manny"),
    ("/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_A/AS_GS_Attack_A_03_B", "A_GS_Attack_A_04_Manny"),
    ("/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_B/AS_GS_Attack_B_01", "A_GS_Attack_B_01_Manny"),
    ("/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_B/AS_GS_Attack_B_02", "A_GS_Attack_B_02_Manny"),
    ("/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_B/AS_GS_Attack_B_03", "A_GS_Attack_B_03_Manny"),
    ("/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_B/AS_GS_Attack_B_04", "A_GS_Attack_B_04_Manny"),
    ("/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_C/AS_GS_Attack_C_01", "A_GS_Attack_C_01_Manny"),
    ("/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_C/AS_GS_Attack_C_02", "A_GS_Attack_C_02_Manny"),
    ("/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_C/AS_GS_Attack_C_03", "A_GS_Attack_C_03_Manny"),
    ("/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_D/AS_GS_Attack_D_01", "A_GS_Attack_D_01_Manny"),
    ("/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_D/AS_GS_Attack_D_02", "A_GS_Attack_D_02_Manny"),
    ("/Game/1dev/OS/SimpleGreatSwordAnim/Animations/Attack/Attack_D/AS_GS_Attack_D_03", "A_GS_Attack_D_03_Manny"),
]

LINES = []


def log(message):
    line = "[GSRetargetUI] {}".format(message)
    LINES.append(line)
    unreal.log(line)


def load_asset(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError("Failed to load asset {}".format(path))
    return asset


def ensure_dir(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def main():
    ensure_dir(ANIM_DIR)
    source_mesh = load_asset(SOURCE_MESH_PATH)
    target_mesh = load_asset(TARGET_MESH_PATH)
    retargeter = load_asset(RETARGETER_PATH)

    for _, target_name in SOURCE_ANIMS:
        target_path = "{}/{}".format(ANIM_DIR, target_name)
        if unreal.EditorAssetLibrary.does_asset_exist(target_path):
            unreal.EditorAssetLibrary.delete_asset(target_path)
            log("Deleted old {}".format(target_path))

    for source_path, _ in SOURCE_ANIMS:
        temp_path = "/Game/{}{}".format(TEMP_PREFIX, source_path.rsplit("/", 1)[-1])
        if unreal.EditorAssetLibrary.does_asset_exist(temp_path):
            unreal.EditorAssetLibrary.delete_asset(temp_path)
            log("Deleted old temp {}".format(temp_path))

    source_asset_data = [unreal.EditorAssetLibrary.find_asset_data(source_path) for source_path, _ in SOURCE_ANIMS]
    result = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
        source_asset_data,
        source_mesh,
        target_mesh,
        retargeter,
        "",
        "",
        TEMP_PREFIX,
        "",
        False,
    )
    log("duplicate_and_retarget returned {} assets".format(len(result)))

    for source_path, target_name in SOURCE_ANIMS:
        source_name = source_path.rsplit("/", 1)[-1]
        temp_path = "/Game/{}{}".format(TEMP_PREFIX, source_name)
        if not unreal.EditorAssetLibrary.does_asset_exist(temp_path):
            raise RuntimeError("Missing retarget output {}".format(temp_path))
        final_path = "{}/{}".format(ANIM_DIR, target_name)
        ok = unreal.EditorAssetLibrary.rename_asset(temp_path, final_path)
        if not ok:
            raise RuntimeError("Failed to move {} -> {}".format(temp_path, final_path))
        asset = load_asset(final_path)
        skeleton = asset.get_editor_property("skeleton")
        skeleton_path = skeleton.get_path_name() if skeleton else "None"
        log("Retargeted {} -> {} skeleton={}".format(source_name, final_path, skeleton_path))
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
        if skeleton_path != TARGET_SKELETON_OBJECT_PATH:
            raise RuntimeError("Wrong skeleton for {}: {}".format(final_path, skeleton_path))

    unreal.EditorAssetLibrary.save_directory(ANIM_DIR)
    log("SUCCESS all GreatSword retargets rebuilt against {}".format(TARGET_SKELETON_PATH))


try:
    main()
except Exception as exc:
    unreal.log_error("[GSRetargetUI] FAILED: {}".format(exc))
    raise
finally:
    os.makedirs(os.path.dirname(OUTPUT_LOG), exist_ok=True)
    with open(OUTPUT_LOG, "w", encoding="utf-8") as handle:
        handle.write("\n".join(LINES))
    unreal.SystemLibrary.quit_editor()
