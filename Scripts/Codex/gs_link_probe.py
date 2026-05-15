import os
import unreal
lines=[]
def log(s):
    lines.append(s)
    unreal.log('[GSLinkProbe] '+s)

def dump_asset(path, props):
    a=unreal.EditorAssetLibrary.load_asset(path)
    if not a:
        log(f'FAILED {path}')
        return
    log(f'asset={path} class={a.get_class().get_name()}')
    for p in props:
        try:
            v=a.get_editor_property(p)
        except Exception as exc:
            v=f'<error:{exc}>'
        log(f'  {p}={v}')


dump_asset('/Game/1dev/OS/DragonKnightRuntime/TestHeroData_DragonRuntime', ['pawn_class','ability_sets','tag_relationship_mapping','input_config'])
dump_asset('/Game/1dev/OS/DragonKnightRuntime/B_Test_ActionCombat_DragonRuntime_Elimination', ['default_pawn_data','action_sets','game_features_to_enable'])
out='D:/UnrealProject/MediaProject/LyraStarterGame/Saved/Codex/gs_link_probe.txt'
os.makedirs(os.path.dirname(out), exist_ok=True)
open(out,'w',encoding='utf-8').write('\n'.join(lines))
