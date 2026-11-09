"""Actual selected linked paths; source/native/client coverage is separate."""
from pathlib import Path
import runpy,json,re,struct
p=runpy.run_path(str(Path(__file__).with_name('verify_seventh_ten_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];ref=p['ref'];values=p['values']
def op(rows,m,s):return any(x.mnemonic==m and x.op_str==s for x in rows)
rows=ins('?_JustDied@BossAI@@');at,=sites(rows,'?KillAllEvents@EventProcessor@@')
assert op(rows[max(0,at-4):at],'xor','edx, edx'),'Boss death must defer referenced events'
plan=json.loads(Path(__file__).with_name('EighthTenRepairs.json').read_text())
for case in plan:
 if not case.get('npc') or case['key']=='WebbedPool':continue
 rows=ins('?JustDied@'+case['cls']+'AI@');at,=sites(rows,'?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@')
 assert any(x.mnemonic=='je' and x.operands[0].imm>rows[at].address for x in rows[:at]),(case['key'],'null killer guard')
 assert op(rows[at+1:at+4],'test','rax, rax') and any(x.mnemonic=='je' for x in rows[at+1:at+4]),(case['key'],'owner guard')
 if case['key']=='Webbed':
  summon,=sites(rows,'?SummonCreature@WorldObject@@');credit,=sites(rows,'?KilledMonsterCredit@Player@@')
  assert summon<at<credit and rows[summon+1].mnemonic=='test' and rows[summon+2].mnemonic=='je'
  draws=sites(rows,'?urand@@');assert len(draws)==2 and op(rows[max(0,draws[1]-4):draws[1]],'mov','edx, 0x1f')
 print('PATH actual selected callback',case['key'])
rows=ins('?OnUpdate@player_hyjal_protectors_random@@');logout=ins('?OnLogout@player_hyjal_protectors_random@@')
assert {5087,29128,5000,96544,96547}<=values(rows)
for body in (rows,logout):
 assert sites(body,'?GetGuidValue@Object@@') and sites(body,'?erase@?$_Tree@V?$_Tmap_traits@VObjectGuid@@UProgress@player_hyjal_protectors_random@@')
 assert op(body,'lea','rbx, [rcx + 0x38]'),'shared recursive mutex address'
# Both callbacks lock/unlock the identical runtime entry points.
from capstone import CS_OP_IMM
branches=lambda body:[x.operands[0].imm for x in body if x.mnemonic in ('call','jmp') and x.operands[0].type==CS_OP_IMM]
a,b=branches(rows),branches(logout);assert a[0]==b[0]
assert any(x.mnemonic=='jmp' and x.operands[0].type==CS_OP_IMM and x.operands[0].imm in b for x in rows),'shared mutex unlock tail'
cast,=sites(rows,'?CastSpell@Unit@@');assert rows[cast+1].mnemonic=='test' and rows[cast+2].mnemonic=='jne'
assert any(x.mnemonic=='bts' for x in rows[cast+3:cast+7]),'accepted slots recorded only after cast OK'
assert op(rows,'test','byte ptr [rsi + 0x40], dl') and op(rows,'cmp','dil, 4')
text=(n['args'].release/'worldserver.map').read_text(errors='replace');m=re.findall(r'\s+\w+:\w+\s+\?\?_7player_hyjal_protectors_random@@6B@\s+([0-9a-fA-F]{16})',text);assert len(m)==1
slots=struct.unpack_from('<55Q',n['binary'],n['offset'](int(m[0],16)))
for method in ('OnUpdate','OnLogout'):assert any(v==n['symbol']('?'+method+'@player_hyjal_protectors_random@@') or v in n['symbols'].values() and n['resolve'](v)==n['symbol']('?'+method+'@player_hyjal_protectors_random@@') for v in slots)
print('PASS actual Release eighth ten:deferred BossAI death event abort,seven guarded owner callbacks,successful Webbed summon before credit and32outcome bound,per-GUID Hyjal map erase/5000ms/4accepted-slot mask/registered update+logout. Prior77 selected paths retained;source/native/client scopes distinct.')
