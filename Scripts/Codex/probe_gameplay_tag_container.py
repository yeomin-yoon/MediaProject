import os
import unreal


OUTPUT_PATH = r"D:\UnrealProject\MediaProject\LyraStarterGame\Saved\Codex\probe_gameplay_tag_container.txt"
LINES = []


def log(message):
    line = "[ProbeTagContainer] {}".format(message)
    LINES.append(line)
    unreal.log(line)


def try_import(text):
    container = unreal.GameplayTagContainer()
    try:
        container.import_text(text)
        log("SUCCESS text={}".format(text))
        log("  export={}".format(container.export_text()))
    except Exception as exc:
        log("FAIL text={} exc={}".format(text, exc))


try_import('(GameplayTags=((TagName="Combat.Input.Held.Modifier")))')
try_import('(GameplayTags=("Combat.Input.Held.Modifier"))')
try_import('{gameplay_tags: ((TagName="Combat.Input.Held.Modifier"))}')
try_import('{gameplay_tags: ("Combat.Input.Held.Modifier")}')

os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
    handle.write("\n".join(LINES))
