"""Check selected Leap hooks, completed-spline damage and native talent consumers."""
from pathlib import Path
import runpy,re,struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_64,CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
p=runpy.run_path(str(Path(__file__).with_name('verify_heroic_leap_transport_release.py')))
n,ins=p['n'],p['instructions']
def refs(method):
    return {x.address+x.size+o.mem.disp for x in ins(method) for o in x.operands if o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP}
def vals(method):return {o.imm for x in ins(method) for o in x.operands if o.type==CS_OP_IMM}
def needs(method,target):
    targets={n['resolve'](v) for k,v in n['symbols'].items() if k.startswith(target)}
    assert n['calls'](method)&targets,(method,target)
outer='?HandleOuterJump@spell_warr_heroic_leap_SpellScript@spell_warr_heroic_leap@@'
missile='?HandleMissile@Script@spell_warr_heroic_leap_jump@@'
for register,handler in [
 ('?Register@spell_warr_heroic_leap_SpellScript@spell_warr_heroic_leap@@',outer),
 ('?Register@Script@spell_warr_heroic_leap_jump@@',missile)]:
    public_addresses=set(n['symbols'].values())
    assert any(n['resolve'](a)==n['symbol'](handler) for a in refs(register) if a in public_addresses), 'Selected hook is not registered'
    # MSVC folds the two identical callbacks and inlines PreventHitDefaultEffect.
    assert any(x.mnemonic=='bts' for x in ins(handler)), 'Default-effect prevention mask absent'
mapping=(n['args'].release/'worldserver.map').read_text(errors='replace')
match=re.search(re.escape('??_7Script@spell_warr_heroic_leap_jump@@6B@')+r'\s+([0-9a-fA-F]{16})',mapping)
assert match
table=int(match[1],16)
assert table in refs('?GetSpellScript@spell_warr_heroic_leap_jump@@')
assert n['symbol']('?Register@Script@spell_warr_heroic_leap_jump@@') in struct.unpack_from('<12Q',n['binary'],n['offset'](table))
assert b'spell_warr_heroic_leap_jump' in n['binary']
final='?Finalize@EffectMovementGenerator@@'
assert {94954,52174}<=vals(final)
body=ins(final)
guard=next(i for i,x in enumerate(body) if x.mnemonic=='cmp' and any(o.type==CS_OP_MEM and o.mem.disp==0x20 and o.size==1 for o in x.operands))
assert body[guard+1].mnemonic=='je'
skip=body[guard+1].operands[0].imm
casttargets={n['resolve'](v) for k,v in n['symbols'].items() if k.startswith('?CastSpell@Unit@@')}
cast=next(x.address for x in body if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm) in casttargets)
assert body[guard].address<cast<skip, 'Uncompleted jump does not skip damage cast'
# Update is a small leaf without x64 unwind metadata; bound by its first ret.
address=n['symbol']('?Update@EffectMovementGenerator@@');decoder=Cs(CS_ARCH_X86,CS_MODE_64);decoder.detail=True
leaf=[]
for x in decoder.disasm(n['binary'][n['offset'](address):n['offset'](address)+64],address):
    leaf.append(x)
    if x.mnemonic=='ret':break
assert leaf[-1].mnemonic=='ret' and any(x.mnemonic=='mov' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0x20 and x.operands[0].size==1 for x in leaf), 'No native completion receipt'
jump='?MoveJump@MotionMaster@@QEAAXMMMM'
assert {94954,52174}<=vals(jump)
needs(jump,'?Launch@MoveSplineInit@Movement@@');needs(jump,'?Mutate@MotionMaster@@')
assert any(x.mnemonic=='jg' for x in ins(jump)), 'Missing positive launch-duration gate'
needs('?EffectJumpDest@Spell@@','?GetSpellTriggerDelay@Spell@@')
needs('?EffectJumpDest@Spell@@','?MoveJump@MotionMaster@@')
needs('?GetSpellCategoryChargesTimer@Player@@','?GetTotalAuraModifierByMiscValue@Unit@@')
needs('?SpellDamageBonusDone@Unit@@','??$ApplySpellMod@M@Player@@')
# IsAffectedBySpellmod is inlined into the native modifier template; the
# extracted production predicates and matching masks are exercised by the host.
assert not any('BilaalDeath' in k for k in n['symbols']), 'Uncoordinated Bilaal WIP in focused build'
print('PASS actual registered suppression hooks, native completed-spline receipt before52174, positive launch gate, real arrival descriptor and native charge/damage/talent-mask consumers; Bilaal WIP excluded. Native/TEMP tests own data and failure cases; client damage/talents remain pending.')
