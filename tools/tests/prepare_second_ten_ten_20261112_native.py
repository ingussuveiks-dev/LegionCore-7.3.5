from pathlib import Path
import sys
from second_ten_ten_20261112_scope import ROOT,PLAN,require_fixed
from prepare_ten_ten_20261111_fifth import block
out=Path(sys.argv[1]);require_fixed()
assert any(r['key']=='PetJournalRemovalCleanup'for r in PLAN['changes'])
source=(ROOT/'src/server/game/Handlers/BattlePetHandler.cpp').read_text(encoding='utf-8-sig')
(out/'SecondJournalRemovalNative.inc').write_text(block(source,r'static void RemoveBattlePetJournalEntry\(')+'\n')
print('Prepared exact complete journal-removal helper; native player/companion/roster APIs remain actual linked implementations.')

if any(row['batch']==8 for row in PLAN['changes']):
 import runpy,re
 saved=sys.argv;sys.argv=[str(ROOT/'tools/tests/prepare_second_ten_ten_20261112_eighth.py'),str(out),'fixed']
 try:runpy.run_path(sys.argv[0],run_name='__main__')
 finally:sys.argv=saved
 rows=[row for row in PLAN['changes']if row['batch']==8]
 source=(ROOT/'src/server/scripts/Legion/AntorusTheBurningThrone/boss_felhounds.cpp').read_text();source+='\n'+(ROOT/'src/server/scripts/Legion/AntorusTheBurningThrone/antorus.h').read_text()
 names=set(re.findall(r'\b(?:SPELL|NPC)_[A-Z_0-9]+\b',rows[9]['edits'][0]['new']));names.discard('SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN');names.discard('SPELL_SCHOOL_MASK_FIRE');names.discard('SPELL_SCHOOL_MASK_SHADOW')
 constants={name:int(re.search(r'\b'+name+r'\s*=\s*(\d+)',source)[1])for name in names}
 aura='namespace SecondEighthAuraNative{enum{'+','.join(k+'='+str(v)for k,v in sorted(constants.items()))+'};\n'
 for row in rows[8:]:aura+=row['edits'][0]['new']+';\n'
 aura=aura.replace('dru->CastCustomSpell(dru, 227034, &finalHeal, nullptr, nullptr, true);','ObserveSecondEighthHeal(dru,dru,227034,finalHeal);').replace('GetTarget()->CastSpell(GetTarget(), SPELL_SHADOWSCAR_FILTER, true);','ObserveSecondEighthCast(GetTarget(),GetTarget(),SPELL_SHADOWSCAR_FILTER);').replace('shatug->CastSpell(GetTarget(), SPELL_BURNING_FLASH, true);','ObserveSecondEighthCast(shatug,GetTarget(),SPELL_BURNING_FLASH);')+'}\n'
 aura+='struct SecondEighthBinding{char const*name;bool spell;};static SecondEighthBinding secondEighthBindings[]={'+','.join('{"'+row['binding']+'",'+('true'if i>=8 else'false')+'}'for i,row in enumerate(rows))+'};\n'
 spells=set()
 for row in rows[:8]:
  text=(ROOT/row['paths'][0]).read_text();body=row['edits'][0]['new']
  for name in set(re.findall(r'\bSPELL_[A-Z_0-9]+\b',body)):
   m=re.search(r'\b'+name+r'\s*=\s*(\d+)',text)
   if m:spells.add(int(m[1]))
 aura+='static uint32 secondEighthSpells[]={'+','.join(map(str,sorted(spells)))+'};\n'
 (out/'EighthAuraNative.inc').write_text(aura)
 print('Prepared actual native EventMap callbacks, full AuraScripts with explicitly observed child-cast endpoints, matching source spell identifiers and database binding names.')

if any(row['batch']==9 for row in PLAN['changes']):
 import runpy,re
 saved=sys.argv;sys.argv=[str(ROOT/'tools/tests/prepare_second_ten_ten_20261112_ninth.py'),str(out),'fixed']
 try:runpy.run_path(sys.argv[0],run_name='__main__')
 finally:sys.argv=saved
 rows=[row for row in PLAN['changes']if row['batch']==9];native='static char const* secondNinthBindings[]={'+','.join('"'+r['binding']+'"'for r in rows)+'};\n';spells=set()
 for row in rows:
  full=(ROOT/row['paths'][0]).read_text();body=row['edits'][0]['new']
  if row['kind']=='combat_repeat'and row['cls']=='boss_bd_nefarianAI':
   body=''
   for label in ['EVENT_SHADOWBLAZE','EVENT_TAIL_LASH']:
    start=row['edits'][0]['new'].index('case '+label+':');end=row['edits'][0]['new'].index('break;',start)+6;body+=row['edits'][0]['new'][start:end]
  else:body='\n'.join(block(body,r'void '+method+r'\(')for method in row['methods'])
  for name in set(re.findall(r'\b(?:SPELL|AURA)_[A-Z_0-9]+\b',body)):
   m=re.search(r'\b'+name+r'\s*=\s*(\d+)',full)
   if m:spells.add(int(m[1]))
 native+='static uint32 secondNinthSpells[]={'+','.join(map(str,sorted(spells)))+'};\n';(out/'NinthBindings.inc').write_text(native)
 print('Prepared actual native reset/reentry EventMap sequences with explicit source projections and matching creature/spell data bindings.')

if any(row['batch']==10 for row in PLAN['changes']):
 import runpy,re
 saved=sys.argv;sys.argv=[str(ROOT/'tools/tests/prepare_second_ten_ten_20261112_tenth.py'),str(out),'fixed']
 try:runpy.run_path(sys.argv[0],run_name='__main__')
 finally:sys.argv=saved
 rows=[r for r in PLAN['changes']if r['batch']==10]
 native='namespace SecondTenthActual{\n'+rows[1]['edits'][0]['new']+';\n'+rows[6]['edits'][0]['new']+';\n'
 clone=rows[2]['edits'][0]['new'];native+='struct Clone{Unit*owner;Creature*me;uint32 entryId=69791;float oldHast=0;'+''.join(block(clone,r'void '+m+r'\(')for m in ['RecalcDamage','RecalcHast'])+'};\n'
 delay=rows[4]['edits'][0]['new'];a=delay.index('caster->AddDelayedEvent');b=delay.index('\n                    });',a)+len('\n                    });');delay=delay[a:b].replace('caster->CastCustomSpell(caster, 247455, nullptr, nullptr, &multiplier, true);','ObserveSecondTenthExplosion(caster, multiplier);')
 native+='static void Schedule(Unit*caster,uint32 fragmentCount){'+delay+'}\n}\n'
 (out/'TenthActualNative.inc').write_text(native)
 bindings='struct SecondTenthBinding{char const*name;char const*table;char const*column;};static SecondTenthBinding secondTenthBindings[]={'+','.join('{"'+r['binding']+'","'+('spell_script_names'if i in[1,4,6]else'areatrigger_scripts'if i==3 else'creature_template')+'","'+('ScriptName')+'"}'for i,r in enumerate(rows)if i in[0,1,2,3,4,6,7])+'};\n'
 (out/'TenthBindings.inc').write_text(bindings)
 print('Prepared final ten source scopes plus actual Aura/Spell/clone methods and exact delayed scheduling block; only child explosion endpoint observed. Queue/GetMaxTicks use actual linked implementations.')
