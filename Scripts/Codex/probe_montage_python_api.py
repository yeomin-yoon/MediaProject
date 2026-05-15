import os
import unreal


OUTPUT_PATH = r"D:\UnrealProject\MediaProject\LyraStarterGame\Saved\Codex\probe_montage_python_api.txt"
LINES = []


def log(message):
    line = "[ProbeMontage] {}".format(message)
    LINES.append(line)
    unreal.log(line)


def safe_get(obj, prop):
    try:
        return obj.get_editor_property(prop)
    except Exception as exc:
        return "<error:{}>".format(exc)


def describe_segment(segment, prefix):
    for prop in ["anim_reference", "start_pos", "anim_start_time", "anim_end_time", "anim_play_rate", "looping_count"]:
        log("{}{}={}".format(prefix, prop, safe_get(segment, prop)))


def main():
    montage = unreal.EditorAssetLibrary.load_asset("/Game/1dev/OS/GDHOneHanded/Montages/AM_GDH_OneHanded_Light01")
    if not montage:
        raise RuntimeError("Failed to load template montage")

    log("montage_class={}".format(montage.get_class().get_name()))
    log("dir_sample={}".format([name for name in dir(montage) if "slot" in name.lower() or "notify" in name.lower() or "section" in name.lower() or "sequence" in name.lower()][:80]))

    for prop in ["slot_anim_tracks", "SlotAnimTracks", "notifies", "Notifies", "composite_sections", "CompositeSections", "sequence_length", "SequenceLength", "blend_in", "blend_out"]:
        value = safe_get(montage, prop)
        if isinstance(value, list):
            log("{}_count={}".format(prop, len(value)))
        else:
            log("{}={}".format(prop, value))

    slot_tracks = getattr(montage, "slot_anim_tracks", None)
    if slot_tracks is None:
        slot_tracks = getattr(montage, "SlotAnimTracks", None)
    if slot_tracks:
        slot = slot_tracks[0]
        log("slot_dir={}".format([name for name in dir(slot) if "slot" in name.lower() or "anim" in name.lower()][:40]))
        log("slot_name={}".format(safe_get(slot, "slot_name")))
        log("slot_name_pascal={}".format(safe_get(slot, "SlotName")))
        anim_track = safe_get(slot, "anim_track")
        if isinstance(anim_track, str) and anim_track.startswith("<error:"):
            anim_track = safe_get(slot, "AnimTrack")
        log("anim_track={}".format(anim_track))
        if hasattr(anim_track, "get_editor_property"):
            log("anim_track_dir={}".format([name for name in dir(anim_track) if "anim" in name.lower() or "segment" in name.lower()][:40]))
            segments = safe_get(anim_track, "anim_segments")
            if isinstance(segments, str) and segments.startswith("<error:"):
                segments = safe_get(anim_track, "AnimSegments")
            log("anim_segments_count={}".format(len(segments) if isinstance(segments, list) else segments))
            if isinstance(segments, list) and segments:
                log("segment0_dir={}".format([name for name in dir(segments[0]) if "anim" in name.lower() or "start" in name.lower() or "loop" in name.lower()][:40]))
                describe_segment(segments[0], "segment0.")

    notifies = getattr(montage, "notifies", None)
    if notifies is None:
        notifies = getattr(montage, "Notifies", None)
    if isinstance(notifies, list):
        for index, notify in enumerate(notifies[:6]):
            log("notify[{}].notify_name={}".format(index, safe_get(notify, "notify_name")))
            log("notify[{}].display_time={}".format(index, safe_get(notify, "display_time")))
            log("notify[{}].duration={}".format(index, safe_get(notify, "duration")))
            log("notify[{}].notify={}".format(index, safe_get(notify, "notify")))
            log("notify[{}].notify_state_class={}".format(index, safe_get(notify, "notify_state_class")))
            log("notify[{}].track_index={}".format(index, safe_get(notify, "track_index")))
            log("notify[{}].montage_tick_type={}".format(index, safe_get(notify, "montage_tick_type")))
            log("notify[{}].link_method={}".format(index, safe_get(notify, "link_method")))

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
        handle.write("\n".join(LINES))


main()
