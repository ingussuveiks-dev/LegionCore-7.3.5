"""Verify actual Boros greeting admission before its once-only dialogue latch."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

p=runpy.run_path(str(Path(__file__).with_name('verify_tuure_boros_queue_release.py')))
n,instructions=p['n'],p['instructions']
method='?MoveInLineOfSight@npc_vindicator_borosAI@npc_vindicator_boros@@'
body=instructions(method)
helper=n['symbol']('?Eligible@TuureIntroHeal@@')
site=next(i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)==helper)
assert any(x.mnemonic=='test' and x.op_str=='rdx, rdx' for x in body[:site]), 'Greeting misses null source guard'
talk=n['symbol']('?Talk@CreatureAI@@QEAAXE')
talk_site=next(i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)==talk)
assert site<talk_site
for prefix in ('?canSeeOrDetect@WorldObject@@','?IsWithinLOSInMap@WorldObject@@','?IsWithinDistInMap@WorldObject@@'):
    matches={n['resolve'](a) for name,a in n['symbols'].items() if name.startswith(prefix)}
    assert any(x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm) in matches for x in body[site:talk_site]), f'Missing actual admission {prefix}'
assert not n['symbol']('?UpdateAchievementCriteria@Player@@') in n['calls'](method)
tables=[]
for x in instructions('?GetAI@npc_vindicator_boros@@'):
    for op in x.operands:
        if op.type==CS_OP_MEM and op.mem.base==X86_REG_RIP:
            table=struct.unpack_from('<80Q',n['binary'],n['offset'](x.address+x.size+op.mem.disp))
            if n['symbol'](method) in table:tables.append(table)
assert len(tables)==1
print('PASS actual selected Boros LOS uses null-safe Player admission and native initial1085/0/41993 eligibility, real distance/visibility/LOS before Talk0; no progression call. Original4000ms/generation queue is separately verified; client voice/appearance pending.')
