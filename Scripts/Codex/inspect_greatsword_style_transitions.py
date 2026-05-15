import os

import unreal


OUTPUT_PATH = r"D:\UnrealProject\MediaProject\LyraStarterGame\Saved\Codex\inspect_greatsword_style_transitions.txt"
STYLE_PATH = "/Game/1dev/ActionCombat/GreatSword/Styles/DA_ActionCombatStyle_GreatSword_Test"
LINES = []


def log(message):
    LINES.append(message)
    unreal.log("[CodexStyleInspect] " + message)


def write_output():
    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
        handle.write("\n".join(LINES))


def tag_text(tag):
    if not tag:
        return "None"
    try:
        exported = tag.export_text()
    except Exception:
        exported = str(tag)
    marker = 'TagName="'
    start = exported.find(marker)
    if start >= 0:
        start += len(marker)
        end = exported.find('"', start)
        if end > start:
            return exported[start:end]
    if exported in ("", "()"):
        return "None"
    return exported


def container_text(container):
    try:
        exported = container.export_text()
    except Exception:
        return str(container)
    tags = []
    marker = 'TagName="'
    start = 0
    while True:
        pos = exported.find(marker, start)
        if pos < 0:
            break
        pos += len(marker)
        end = exported.find('"', pos)
        if end < 0:
            break
        tags.append(exported[pos:end])
        start = end + 1
    return ",".join(tags) if tags else "None"


def obj_path(obj):
    if not obj:
        return "<none>"
    try:
        return obj.get_path_name()
    except Exception:
        return str(obj)


def main():
    style = unreal.EditorAssetLibrary.load_asset(STYLE_PATH)
    if not style:
        raise RuntimeError(f"failed to load {STYLE_PATH}")

    log(f"style={style.get_path_name()}")

    actions = list(style.get_editor_property("actions"))
    log(f"actions.count={len(actions)}")
    for index, action in enumerate(actions):
        log(
            f"A[{index}] tag={tag_text(action.get_editor_property('action_tag'))} "
            f"montage={obj_path(action.get_editor_property('montage'))}"
        )

    transitions = list(style.get_editor_property("transitions"))
    log(f"transitions.count={len(transitions)}")
    for index, transition in enumerate(transitions):
        log(
            f"T[{index}] from={tag_text(transition.get_editor_property('from_action_tag'))} "
            f"command={tag_text(transition.get_editor_property('command_tag'))} "
            f"to={tag_text(transition.get_editor_property('to_action_tag'))} "
            f"requiredHeld={container_text(transition.get_editor_property('required_held_input_tags'))} "
            f"blockedHeld={container_text(transition.get_editor_property('blocked_held_input_tags'))}"
        )


try:
    main()
finally:
    write_output()
