import unreal

STYLE_PATHS = [
    "/Game/1dev/OS/GDHOneHanded/PrimaryAttack_GDHOneHanded_Auto",
    "/Game/1dev/ActionCombat/GreatSword/Styles/DA_ActionCombatStyle_GreatSword_Test",
]

OUTPUT_PATH = r"D:/UnrealProject/MediaProject/LyraStarterGame/Saved/Codex/retune_greatsword_rootmotion.txt"
ASSET_LIB = unreal.EditorAssetLibrary


def action_tag_text(action):
    tag = action.get_editor_property("action_tag")
    return tag.export_text() if tag else ""


def tune_action(action, lines):
    attack_advance = action.get_editor_property("attack_advance")
    attack_advance.set_editor_property("enabled", False)
    action.set_editor_property("attack_advance", attack_advance)

    montage = action.get_editor_property("montage")
    sequence = montage.get_first_anim_reference() if montage else None
    lines.append(f"ACTION {action_tag_text(action)}")
    lines.append(f"  montage={montage.get_path_name() if montage else 'None'}")
    lines.append(f"  sequence={sequence.get_path_name() if sequence else 'None'}")

    if sequence:
        sequence.set_editor_property("enable_root_motion", True)
        sequence.set_editor_property("force_root_lock", False)
        ASSET_LIB.save_loaded_asset(sequence)
        lines.append("  root_motion=enabled")
        lines.append("  force_root_lock=False")


def main():
    lines = []

    for style_path in STYLE_PATHS:
        style = ASSET_LIB.load_asset(style_path)
        lines.append(f"STYLE {style_path}")
        if not style:
            lines.append("  missing")
            continue

        actions = style.get_editor_property("actions")
        updated = 0
        for action in actions:
            if "Combat.Action.GreatSword." not in action_tag_text(action):
                continue
            tune_action(action, lines)
            updated += 1

        style.set_editor_property("actions", actions)
        ASSET_LIB.save_loaded_asset(style)
        lines.append(f"  updated_actions={updated}")

    with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines))

    print(f"WROTE {OUTPUT_PATH}")


main()
