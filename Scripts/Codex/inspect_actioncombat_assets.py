import os
import unreal


OUTPUT_PATH = r"D:\UnrealProject\MediaProject\LyraStarterGame\Saved\Codex\inspect_actioncombat_assets.txt"
LINES = []


def log(msg):
    line = f"[CodexInspect] {msg}"
    LINES.append(line)
    unreal.log(line)


def safe_get(obj, name):
    try:
        return obj.get_editor_property(name)
    except Exception as exc:
        return f"<error:{exc}>"


def list_props(obj, names):
    for name in names:
        value = safe_get(obj, name)
        log(f"{obj.get_name()}.{name} = {value}")


def main():
    asset_paths = [
        "/Game/1dev/OS/GDHOneHanded/PrimaryAttack_GDHOneHanded_Auto",
        "/Game/1dev/OS/TestHeroData_ActionCombatOnly",
        "/Game/1dev/OS/B_Test_ActionCombat_Elimination",
    ]

    for path in asset_paths:
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if not asset:
            log(f"FAILED load {path}")
            continue

        log(f"asset={path} class={asset.get_class().get_name()}")

        if asset.get_class().get_name() == "ActionCombatStyleData":
            actions = safe_get(asset, "actions")
            transitions = safe_get(asset, "transitions")
            log(f"  actions={len(actions)} transitions={len(transitions)}")
            for action in actions[:5]:
                action_tag = str(action.action_tag)
                montage = action.montage.get_name() if action.montage else "<none>"
                log(f"  action {action_tag} montage={montage}")

        referencers = unreal.EditorAssetLibrary.find_package_referencers_for_asset(path, False)
        log(f"  referencers={referencers}")

        if asset.get_class().get_name().endswith("Blueprint"):
            generated_class = safe_get(asset, "generated_class")
            log(f"  generated_class={generated_class}")

    log("done")

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
        handle.write("\n".join(LINES))


main()
