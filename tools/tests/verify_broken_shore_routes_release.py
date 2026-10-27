"""Check class-specific map1500 admission and checkpoint routing in the PE."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM
p=runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
n=p['native']
route='?Dungeon@BrokenShoreClassRoute@@'
assert n['symbol'](route) in n['calls']('?AddPlayerToMap@InstanceMap@@'), 'Instance admission does not select class dungeon'
resume=[name for name in n['symbols'] if name.startswith('?ResumeDungeon@')]
assert len(resume)==1 and n['symbol'](route) in n['calls'](resume[0]), 'Checkpoint bypasses class routing'
values={op.imm for ins in p['instructions'](route) for op in ins.operands if op.type==CS_OP_IMM}
assert {1500,775,933,961,906,1176,1166}<=values, 'Missing native map/scenario/dungeon identities'
assert b'Broken Shore class route missing for class %u difficulty %u; refusing foreign scenario' in n['binary']
assert b'Broken Shore class route rejected live scenario %u for class %u' in n['binary'], 'Existing foreign-instance guard absent'
print('PASS canonical native map admission/checkpoint share strict map1500 class route with actual775/933/961 and906/1176/1166 identities. Client restart/traversal remains separate.')
