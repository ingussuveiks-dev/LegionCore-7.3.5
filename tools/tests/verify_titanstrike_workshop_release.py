from pathlib import Path
import runpy
n = runpy.run_path(str(Path(__file__).with_name('verify_titanstrike_runes_release.py')))
constants = n['immediates']('?UpdateAI@npc_titanstrike_xr947@@')
assert {382302,382300,382303,106352,106354,210845,210836,106376,14150,9002055}.issubset(constants)
assert 187207 in n['immediates']('?UpdateAI@npc_titanstrike_prustaga@@')
assert {210836,69}.issubset(n['immediates']('?UpdateAI@npc_titanstrike_grif@@'))
assert n['native']['symbol']('?SetData@SmartAI@@') in n['native']['calls']('?UpdateAI@npc_titanstrike_xr947@@')
print('PASS: canonical workshop callback tracks all three DB actors, retries activation, retains native dispatch/hologram/conversation and correct ally roles')
