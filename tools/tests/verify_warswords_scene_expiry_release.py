"""Selected native finite aura end reaches the authenticated existing scene bridge."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM
from five_ten_20261111_pe import ins as bounded_ins
from warswords_scene_expiry_scope import verify
verify()
p=runpy.run_path(str(Path(__file__).with_name('verify_warswords_attackability_release.py')))
n,ins=p['n'],p['ins'];sites=p['p']['sites']
effect=ins('?HandleAuraActivateScene@AuraEffect@@')
complete,=sites(effect,'?CompleteActiveSpellScene@Player@@');cancel,=sites(effect,'?SendSpellScene@Player@@')
assert complete<cancel,'Expiry completion must precede ordinary native cancellation'
values={o.imm for x in effect[:complete]for o in x.operands if o.type==CS_OP_IMM}
assert {205443,1218,1511,1580,205340,1215,1495,1575,21}<=values,'Do not widen spell/scene/map/native metadata scope'
for dependency in ('?GetSpellScene@SpellMgr@@',):
 assert sites(effect,dependency),dependency
mode,=[i for i,x in enumerate(effect)if x.mnemonic=='cmp'and x.op_str=='byte ptr [rbx + 0x10], 4']
assert mode<complete and ('mov','rbx, rdx')in {(x.mnemonic,x.op_str)for x in effect[:mode]},'Actual AuraApplication argument/remove-mode byte must gate expiry'
reaches=p['p']['p']['reachable']
assert effect[mode+1].mnemonic=='jne'and effect[complete].address not in reaches(effect,effect[mode+1].operands[0].imm),'Non-expiry removal reaches completion'
active=bounded_ins('?CompleteActiveSpellScene@Player@@')
assert {('mov','r9d, edx'),('xor','edx, edx'),('cmp','dword ptr [rax + 0x14], r9d'),('cmova','edx, dword ptr [rax + 0x10]')}<={(x.mnemonic,x.op_str)for x in active},'Keep latest matching active scene,not an arbitrary instance'
tail,=[i for i,x in enumerate(active)if x.mnemonic=='jne'and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)==n['symbol']('?SceneCompleted@Player@@')]
assert active[tail-1].op_str=='edx, edx'and active[tail+1].mnemonic=='ret','Only a nonzero real receipt may tail-dispatch native completion'
for rows in (effect,active):
 for prefix in ('?AddItem@Player@@','?CompleteQuest@Player@@','?TeleportTo@Player@@','?UpdateAchievementCriteria@Player@@'):
  assert not sites(rows,prefix),'Finite expiry must retain authenticated script/item/quest endpoints'
assert n['symbol']('?TrigerScene@Player@@')in n['calls']('?SceneCompleted@Player@@')
print('PASS linked exact finite Fury205443/1218 and Protection205340/1215 expiry acknowledges latest active receipt before native cancellation;existing authenticated scene script/equip/return retained. Early cancellation and other scenes remain ordinary;native/client limits separate.')
