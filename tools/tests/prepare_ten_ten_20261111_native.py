from pathlib import Path
import re,sys
from ten_ten_20261111_scope import ROOT,PLAN,require_fixed
out=Path(sys.argv[1]);require_fixed();e=next(e for e in PLAN['changes']if e['key']=='SinestraIndomitableIsolation');source=e['edits'][0]['new'];assert source.count('GetCaster()->CastSpell(GetTarget(), SPELL_INDOMITABLE_DMG, true);')==1
source=source.replace('GetCaster()->CastSpell(GetTarget(), SPELL_INDOMITABLE_DMG, true);','Observe::CastSpell(GetCaster()->ToCreature(), GetTarget(), SPELL_INDOMITABLE_DMG, true);')
(out/'TenTenFirstNative.inc').write_text('namespace TenTenFirst{enum{SPELL_INDOMITABLE_DMG=90045};'+source+';}\n',encoding='utf8')
print('Prepared actual AuraScript class with a controlled child-cast endpoint;all state/native getters/periodic registration retained')
second=[e for e in PLAN['changes']if e['batch']==2]
if second:
 assert len(second)==10
 protection=next(e for e in second if e['case']=='absorb')['edits'][0]['new']
 assert protection.count('target->CastSpell(target,')==2
 protection=protection.replace('target->CastSpell(target,','ObserveProtection(target, target,')
 metadata='struct SecondBinding{uint32 spell;char const*name;bool scaling;};static SecondBinding secondBindings[]={'+','.join('{'+str(spell)+',"'+e['binding']+'",'+('true'if e['case']=='scaling'else'false')+'}'for e in second if 'binding'in e for spell in e['spells'])+'};\n'
 commands='static char const*secondCommands[]={'+','.join('"'+command[0]+' 123"'for command in second[0]['commands'])+'};\n'
 (out/'TenTenSecondNative.inc').write_text(protection+';\n'+metadata+commands,encoding='utf8')
 print('Prepared actual protection AuraScript with observed child casts and native binding/command metadata for batch2')
third=[e for e in PLAN['changes']if e['batch']==3]
if third:
 assert len(third)==10
 metadata='struct ThirdBinding{char const*kind;char const*name;};static ThirdBinding thirdBindings[]={'+','.join('{"'+e['binding_kind']+'","'+e['binding']+'"}'for e in third)+'};\n'
 e=next(e for e in third if e['case']=='shatter');source=(ROOT/e['paths'][0]).read_text(encoding='utf-8-sig');start=source.index('class spell_eonar_shatter :');end=source.index('\n};',start)+3;body=source[start:end]
 assert body.count('caster->CastSpell(caster,')==4
 body=body.replace('caster->CastSpell(caster,','ObserveProtection(caster, caster,')
 definitions=source+'\n'+(ROOT/'src/server/scripts/Legion/AntorusTheBurningThrone/antorus.h').read_text(encoding='utf-8-sig')
 labels=['NPC_CRYSTAL_TARGETED','NPC_CRYSTAL_BURNING','NPC_CRYSTAL_FOUL_STEPS','NPC_CRYSTAL_ARCANE_SINGULARITY','SPELL_TARGETED','SPELL_BURNING_EMBERS','SPELL_FOUL_STEPS','SPELL_ARCANE_SINGULARITY']
 enums='enum{ACTION_2=2,'+','.join(label+'='+re.search(r'\b'+label+r'\s*=\s*(\d+)',definitions)[1]for label in labels)+'};\n'
 (out/'TenTenThirdNative.inc').write_text(metadata+'namespace ThirdNative{'+enums+body+'}\n',encoding='utf8')
 print('Prepared third10 exact binding metadata and actual Eonar AuraScript/native constants;child cast endpoint observed')
