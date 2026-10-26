"""Match Tuure eligibility to the actual native Scenario criteria GM gate."""
from pathlib import Path
import runpy
from capstone import CS_OP_MEM,CS_OP_IMM

p=runpy.run_path(str(Path(__file__).with_name('verify_tuure_boros_greeting_release.py')))
n,instructions=p['n'],p['instructions']
methods=('?Eligible@TuureIntroHeal@@','?Eligible@TuureDefeat@@','?CanTake@TuureAcquisition@@','?Complete@TuurePresentation@@')
guards=[]
for method in methods:
    body=instructions(method)
    candidates=[(i,x) for i,x in enumerate(body) if x.mnemonic=='test' and any(op.type==CS_OP_MEM for op in x.operands) and any(op.type==CS_OP_IMM and op.imm==1 for op in x.operands)]
    assert len(candidates)==1
    site,guard=candidates[0];member=next(op for op in guard.operands if op.type==CS_OP_MEM)
    assert body[site+1].mnemonic=='jne', f'GM not rejected in {method}'
    guards.append(member.mem.disp)
    owner_life=[x for x in body for op in x.operands if op.type==CS_OP_MEM and op.mem.base==member.mem.base and op.mem.disp==0xa18]
    assert bool(owner_life)==(method!='?Eligible@TuureDefeat@@'), 'Defeat alone must admit native dead owner; physical interactions retain alive gate'
assert len(set(guards))==1
native=[name for name in n['symbols'] if name.startswith('?UpdateAchievementCriteria@?$AchievementMgr@VScenario@@@@') and 'AchievementCache' in name]
assert len(native)==1
body=instructions(native[0])
assert any(x.mnemonic=='test' and any(op.type==CS_OP_MEM and op.mem.disp==guards[0] for op in x.operands) and any(op.type==CS_OP_IMM and op.imm==1 for op in x.operands) for x in body), 'Scripts do not match native GM flag'
assert n['symbol']('?CanContact@Player@@') in n['calls'](native[0])
print('PASS four selected Tuure eligibility helpers test the same actual native Scenario GM flag; combat recipient alone omits owner death-state rejection while initial heal/pickup/scene retain alive guards. Native GM/contact gate unchanged. Native catalog/host owns exact criteria metadata and dead-owner/allied/pet cases; client pending.')
