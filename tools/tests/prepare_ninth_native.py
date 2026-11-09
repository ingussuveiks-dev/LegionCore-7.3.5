from pathlib import Path
import json,re,sys
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);plan=json.loads((r/'tools/tests/NinthTenRepairs.json').read_text());parts=[]
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
for key in ['Divine','FrostShield','Miasma','ManaBarrier']:
 e=next(e for e in plan if e['key']==key);s=(r/e['paths'][0]).read_text(encoding='utf-8-sig');part=block(s,'class '+e['cls']+' :')+';';part=re.sub(r'(PrepareAuraScript\([^;]+\);)',r'\1\npublic:',part);prefix=''
 names={'Divine':['PALADIN_SPELL_FORBEARANCE'],'TimeLoop':['SPELL_TIMELOOP_HEAL'],'Miasma':['SPELL_ANCIENT_BARRIER','SPELL_ANCIENT_MIASMA_DMG','SPELL_ANCIENT_MIASMA_H_A']} .get(key,[])
 if key=='Miasma':
  prefix+=re.search(r'uint32 ancientbarrierbar\[3\]\s*=\s*\{[^}]+\};',s).group()+'\n'
  names+=re.findall(r'\bSP[EL]+_\w+',prefix)
 for name in sorted(set(names)):
  m=re.search(r'\b'+name+r'\s*=\s*(\d+)',s);assert m,name;prefix='constexpr uint32 '+name+'='+m[1]+';\n'+prefix
 parts.append('namespace N'+key+'{'+prefix+part+'}')
# Native dataflows retain original source statements, not copied arithmetic.
for key in ['Decimate','Alar','Garrosh','Immerseus']:
 e=next(e for e in plan if e['key']==key);s=(r/e['paths'][0]).read_text(encoding='utf-8-sig')
 if key=='Decimate':body=block(s,'void HandleScript(',s.index('class spell_stinky_precious_decimate'))if False else block(s[s.index('class spell_stinky_precious_decimate'):],'void HandleScript(');parts.append('struct NDecimate{Unit*target;int32 value;Unit*GetHitUnit(){return target;}int32 GetEffectValue(){return value;}'+body+'};')
 if key=='Alar':
  body=re.search(r'uint64 health = Alar->GetHealth\(\);\s*uint64 loss = Alar->CountPctFromMaxHealth\(3\);\s*Alar->SetHealth\(health > loss \? health - loss : 1\);',s).group();parts.append('static void NAlar(Unit*Alar){'+body+'}')
 if key=='Garrosh':
  body=re.search(r'uint64 hp = me->GetHealth\(\);\s*uint32 power = me->GetPower\(POWER_ENERGY\);\s*garroshrealm->SetHealth\(hp\);\s*garroshrealm->SetPower\(POWER_ENERGY, power\);',s).group();parts.append('static void NGarrosh(Unit*me,Unit*garroshrealm){'+body+'}')
 if key=='Immerseus':
  body=re.search(r'uint64 modh = me->CountPctFromMaxHealth\(float\(doneval\)\);\s*if \(lasthp > modh\)\s*me->SetHealth\(lasthp - modh\);\s*else\s*me->SetHealth\(me->CountPctFromMaxHealth\(1\)\);',s).group();parts.append('static void NImmerseus(Unit*me,uint64 lasthp,uint8 doneval){'+body+'}')
(out/'NinthNativeScripts.inc').write_text('\n'.join(parts),encoding='utf8')
