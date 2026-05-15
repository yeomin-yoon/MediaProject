import os
import unreal


OUTPUT_PATH = r"D:\UnrealProject\MediaProject\LyraStarterGame\Saved\Codex\probe_anim_sequence_api.txt"
LINES = []


def log(message):
    line = "[ProbeAnimSequence] {}".format(message)
    LINES.append(line)
    unreal.log(line)


def safe_get(obj, prop):
    try:
        return obj.get_editor_property(prop)
    except Exception as exc:
        return "<error:{}>".format(exc)


def main():
    sequence = unreal.EditorAssetLibrary.load_asset("/Game/1dev/OS/GDHOneHanded/Animations/A_GDH_OneHanded_Light01")
    if not sequence:
        raise RuntimeError("Failed to load sequence")

    log("sequence_class={}".format(sequence.get_class().get_name()))
    log("dir_sample={}".format([name for name in dir(sequence) if "notify" in name.lower() or "track" in name.lower() or "length" in name.lower()][:120]))

    for prop in [
        "notifies",
        "Notifies",
        "animation_track_names",
        "AnimationTrackNames",
        "sequence_length",
        "SequenceLength",
    ]:
        value = safe_get(sequence, prop)
        if hasattr(value, "__len__") and not isinstance(value, str):
            try:
                log("{}_count={}".format(prop, len(value)))
            except Exception:
                log("{}={}".format(prop, value))
        else:
            log("{}={}".format(prop, value))

    notifies = getattr(sequence, "notifies", None)
    if notifies is None:
        notifies = getattr(sequence, "Notifies", None)
    if hasattr(notifies, "__len__") and not isinstance(notifies, str):
        for index, notify in enumerate(notifies[:6]):
            log("notify[{}].dir={}".format(index, [name for name in dir(notify) if "notify" in name.lower() or "time" in name.lower() or "duration" in name.lower()][:40]))
            for prop in ["notify_name", "display_time", "duration", "notify", "notify_state_class", "track_index"]:
                log("notify[{}].{}={}".format(index, prop, safe_get(notify, prop)))

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
        handle.write("\n".join(LINES))


main()
