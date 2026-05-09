import os
import unreal


OUTPUT_PATH = r"D:\UnrealProject\MediaProject\LyraStarterGame\Saved\Codex\probe_notify_libraries.txt"
LINES = []


def log(message):
    line = "[ProbeNotifyLibs] {}".format(message)
    LINES.append(line)
    unreal.log(line)


def dump_type(name):
    obj = getattr(unreal, name, None)
    log("type {} exists={}".format(name, obj is not None))
    if obj is None:
        return
    names = [entry for entry in dir(obj) if not entry.startswith("_")]
    log("  names={}".format(names[:200]))


def main():
    for name in [
        "AnimationLibrary",
        "AnimDataController",
        "AnimDataModel",
        "UAnimNotifyLibrary",
        "UAnimNotifyStateMachineInspectionLibrary",
        "UAnimNotifyMirrorInspectionLibrary",
    ]:
        dump_type(name)

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
        handle.write("\n".join(LINES))


main()
