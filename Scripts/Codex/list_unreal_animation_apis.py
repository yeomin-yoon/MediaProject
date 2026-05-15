import os
import unreal


OUTPUT_PATH = r"D:\UnrealProject\MediaProject\LyraStarterGame\Saved\Codex\list_unreal_animation_apis.txt"

names = sorted([name for name in dir(unreal) if "anim" in name.lower() or "notify" in name.lower() or "sequence" in name.lower() or "montage" in name.lower()])

os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
    handle.write("\n".join(names))
