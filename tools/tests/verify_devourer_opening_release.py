"""Actual registered final-Akaari1942 owner/corpse-independent live actor."""
from pathlib import Path
import re
import runpy
import struct
from sharasdal_erunak_entry_pe import sites, values
from merlis_intro_pe import reachable
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
p=runpy.run_path(str(Path(__file__).with_name('verify_devourer_combat_release.py')))
n=p['n'];ins=p['p']['ins'];method=p['method'];pairs=p['pairs']
suffix='OpeningConversation@npc_akaari_shadowgore_lastAI@npc_akaari_shadowgore_last@@'
text=(n['args'].release/'worldserver.map').read_text(errors='replace')
def table(name):
    matches=re.findall(r'\s+\w+:\w+\s+'+re.escape(name)+r'\s+([0-9a-fA-F]{16})',text)
    assert len(matches)==1,name
    return int(matches[0],16)
def refs(body):
    return {op.address+op.size+v.mem.disp for op in body for v in op.operands
            if v.type==CS_OP_MEM and v.mem.base==X86_REG_RIP}
base=table('??_7Conversation@@6BWorldObject@@@');derived=table('??_7'+suffix+'6BWorldObject@@@')
raw=struct.unpack_from('<50Q',n['binary'],n['offset'](base))
find_slot,=[i for i,at in enumerate(raw) if n['resolve'](at)==n['symbol']('?FindCreatureActor@Conversation@@')]
update_slot,=[i for i,at in enumerate(raw[:find_slot]) if n['resolve'](at)==n['symbol']('?Update@Conversation@@')]
actual=struct.unpack_from('<'+str(find_slot+1)+'Q',n['binary'],n['offset'](derived))
for slot,name in [(1,'AddToWorld'),(find_slot,'FindCreatureActor'),(update_slot,'Update')]:
    assert n['resolve'](actual[slot])==n['symbol']('?'+name+'@'+suffix)
for i in range(2,find_slot):
    if i!=update_slot:assert n['resolve'](actual[i])==n['resolve'](raw[i])
voice=p['voice'];enter=p['enter'];owner=ins(method('OpeningCaster'))
assert derived in refs(voice) and 0x528 in values(voice)
assert {('movups','xmm6, xmmword ptr [rax]'),('movups','xmmword ptr [r14 + 0x508], xmm6'),
        ('mov','qword ptr [r14 + 0x518], rax')} <= pairs(voice)
lookup,=sites(n,owner,'?GetPlayer@ObjectAccessor@@');identity,=sites(n,owner,'??9ObjectGuid@@')
receipt,=sites(n,owner,'?GetQuestCancellationReceipt@Player@@')
assert lookup<identity<receipt and owner[identity+2].mnemonic=='jne'
assert owner[receipt].address not in reachable(owner,owner[identity+2].operands[0].imm)
assert {('mov','rax, qword ptr [rcx + 0xb0]'),('movzx','eax, byte ptr [rax]'),
        ('cmp','qword ptr [rdi + 0xb0], rsi')} <= pairs(owner)
for name in ['?CanContact@Player@@','?IsInWorld@Object@@','?IsPreDelete@Object@@',
             '?IsQuestObjectExpired@WorldObject@@','?IsWithinDistInMap@WorldObject@@',
             '?InSamePhase@WorldObject@@','?InSamePhaseId@WorldObject@@Q','?CanTargetQuestObject@WorldObject@@']:
    assert sites(n,owner,name),name
native_owner,=sites(n,enter,'?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@')
assert any(op.mnemonic=='test' and op.op_str=='rdi, rdi' and enter[i+1].mnemonic=='je' for i,op in enumerate(enter[:native_owner]))
assert {('movups','xmmword ptr [rsi + 0x9f], xmm0'),('mov','dword ptr [rsi + 0x68], 0x796'),
        ('mov','byte ptr [rsi + 0x9c], 1')} <= pairs(enter)
assert sites(n,enter,'?GetQuestCancellationReceipt@Player@@') and len(sites(n,voice,method('OpeningCaster')))==2
assert len(sites(n,voice,'?Remove@Conversation@@'))>=2
actor=ins('?FindCreatureActor@'+suffix);admit=ins('?AddToWorld@'+suffix);update=ins('?Update@'+suffix)
assert {105660,1607} <= values(actor) and ('test','edx, edx') in pairs(actor)
for rows in (actor,admit):
    assert sites(n,rows,'?GetCreature@ObjectAccessor@@')
    assert ('cmp','dword ptr [rbx + 0xa18], 0') in pairs(rows) # Native death state must be ALIVE.
identity,=sites(n,actor,'??9ObjectGuid@@');assert actor[identity+2].mnemonic=='jne'
success,=[op.address for op in actor if (op.mnemonic,op.op_str)==('mov','rax, rbx')]
assert success not in reachable(actor,actor[identity+2].operands[0].imm)
identity,=sites(n,admit,'??8ObjectGuid@@');scope,=sites(n,admit,'?InheritQuestObjectScope@WorldObject@@');publish,=sites(n,admit,'?AddToWorld@Conversation@@')
assert identity<scope<publish and admit[publish].address not in reachable(admit,admit[identity+2].operands[0].imm)
assert ('mov','rcx, qword ptr [rdi + 0x518]') in pairs(admit)
assert ('mov','rax, qword ptr [rcx + 0x518]') in pairs(update)
assert sites(n,update,'?Remove@Conversation@@') and sites(n,update,'?Update@Conversation@@')
caster_lookup,=sites(n,update,'?GetUnit@ObjectAccessor@@')
caster_identity,=sites(n,update,'??9ObjectGuid@@')
assert caster_lookup<caster_identity<sites(n,update,'?IsInWorld@Object@@')[0]
assert sites(n,update,'?IsPreDelete@Object@@')
assert {('mov','r14d, dword ptr [rdi + 0x4dc]'), # Actual published duration.
        ('sub','eax, dword ptr [rdi + 0x500]'), # Native creation clock.
        ('sub','ecx, dword ptr [rax + 0x2c]'), # Each admitted viewer clock.
        ('cmp','ecx, r14d')} <= pairs(update)
viewer_hold,=[i for i,op in enumerate(update) if (op.mnemonic,op.op_str)==('cmp','ecx, r14d')]
assert update[viewer_hold+1].mnemonic=='jbe'
held=reachable(update,update[viewer_hold+1].operands[0].imm)
assert all(update[i].address not in held for i in sites(n,update,'?Remove@Conversation@@'))
assert 7800 not in values(update), 'Cleanup must use admitted native duration, not a replacement timer'
for rows in (owner,actor,enter):assert not sites(n,rows,'?FindNearestCreature@WorldObject@@')
print('PASS registered1942 opening: native Player/owner lookup/full128-bit captured identity/current quest receipt; exact live source/publication/source scope, receipt/caster removal, native duration/creation/viewer clocks and full admitted-viewer hold before bounded missing-playback cleanup, native personal/group or NPC fallback, same voice lane retry/original ability clocks. Actual client playback/full route separate.')
