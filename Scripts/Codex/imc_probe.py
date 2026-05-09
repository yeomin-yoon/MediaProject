import os
import unreal
lines=[]
def log(s):
    lines.append(s)
    unreal.log('[IMCProbe] '+s)

for path in ['/Game/Input/Mappings/IMC_Default','/Game/1dev/OS/Test_InputData_Hero']:
    asset=unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        log(f'FAILED {path}')
        continue
    log(f'asset={path} class={asset.get_class().get_name()}')
    for prop in ['mappings','native_input_actions','ability_input_actions']:
        try:
            value=asset.get_editor_property(prop)
        except Exception as exc:
            log(f'  {prop}=<error:{exc}>')
            continue
        log(f'  {prop}.count={len(value)}')
        for i,item in enumerate(value[:40]):
            log(f'    [{i}] {item}')
out='D:/UnrealProject/MediaProject/LyraStarterGame/Saved/Codex/imc_probe.txt'
os.makedirs(os.path.dirname(out), exist_ok=True)
open(out,'w',encoding='utf-8').write('\n'.join(lines))
