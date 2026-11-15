"""Selected real PE calls/registration for the marked179109 reconstruction."""
from pathlib import Path
import bisect,hashlib,json,runpy,struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_64,CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
n=runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
d=Cs(CS_ARCH_X86,CS_MODE_64);d.detail=True
def instructions(prefix):
 address=n['symbol'](prefix);i=bisect.bisect_right(n['starts'],address-n['base'])-1
 begin,end,unwind=n['functions'][i]
 assert begin<=address-n['base']<end
 origin=n['root'](begin,unwind)
 for b,e,u in n['functions'][i+1:]:
  if b!=end or n['root'](b,u)!=origin:break
  end=e
 at=n['offset'](address)
 return list(d.disasm(n['binary'][at:at+n['base']+end-address],address))
def targets(body):
 return {n['resolve'](x.operands[0].imm) for x in body if x.mnemonic in ('call','jmp') and x.operands[0].type==CS_OP_IMM}
def require(prefix,calls=(),values=()):
 body=instructions(prefix);t=targets(body)
 for call in calls:assert n['symbol'](call) in t,(prefix,call)
 constants={op.imm for x in body for op in x.operands if op.type==CS_OP_IMM}
 assert set(values)<=constants,(prefix,set(values)-constants)
 return body
guard=require('?Guard@ArcaneEggLoot@@',('?GetGameObject@Map@@','?InSamePhase@WorldObject@@','?GetAttackDistance@Creature@@','?IsValidAttackTarget@Unit@@','?isInCombat@Unit@@'),(1220,239692,89652))
defend=require('?Defend@spell_arcane_infused_egg@ArcaneEggLoot@@',('?GetCaster@SpellScript@@','?Guard@ArcaneEggLoot@@'))
update=require('?UpdateAI@npc_arcane_egg_heron@ArcaneEggLoot@@',('?UpdateAI@VehicleAI@@','?UpdateVictim@CreatureAI@@','?DoMeleeAttackIfReady@UnitAI@@'))
# The override may be folded into a single tail jump to native AttackStart.
attack=n['symbol']('?AttackStart@npc_arcane_egg_heron@ArcaneEggLoot@@')
assert attack==n['symbol']('?AttackStart@UnitAI@@') or n['symbol']('?AttackStart@UnitAI@@') in n['calls']('?AttackStart@npc_arcane_egg_heron@ArcaneEggLoot@@')
registration=instructions('?AddSC_azsuna@@');installed=set()
for x in registration:
 for op in x.operands:
  if op.type==CS_OP_MEM and op.mem.base==X86_REG_RIP:
   address=x.address+x.size+op.mem.disp
   try:installed.update(struct.unpack_from('<32Q',n['binary'],n['offset'](address)))
   except (ValueError,struct.error):pass
for prefix in ('?GetSpellScript@?$GenericSpellScriptLoader@Vspell_arcane_infused_egg@ArcaneEggLoot@@','?GetAI@?$GenericCreatureScript@Unpc_arcane_egg_heron@ArcaneEggLoot@@'):
 assert n['symbol'](prefix) in installed,('Actual zone registration missing',prefix)
for body in (guard,defend,update):
 for prefix in ('?AddItem@Player@@','?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?setFaction@Unit@@'):
  assert not {n['resolve'](a) for name,a in n['symbols'].items() if name.startswith(prefix)}&targets(body)
print('PASS linked egg source/guard checks, actual two zone factories, native melee and retained VehicleAI update; no item/quest/faction grants')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'scope':'selected linked implementation; native and client evidence separate'}))
