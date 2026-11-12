"""Exact affected production decisions, with untouched encounter endpoints explicit."""
from pathlib import Path
import re,json
from ten_ten_20261111_scope import ROOT,PLAN,require_fixed

def block(s,pattern):
 masked=re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"',lambda m:re.sub(r'[^\n]',' ',m[0]),s,flags=re.S);m=re.search(pattern,masked);assert m,pattern;a=masked.index('{',m.start());i=a+1;d=1
 while d:d+=(masked[i]=='{')-(masked[i]=='}');i+=1
 return s[m.start():i]
def selected(e,mode):
 p=e['paths'][0];s=(ROOT/p).read_text(encoding='utf8',errors='surrogateescape')
 if mode=='baseline':
  for ed in reversed(e['edits']):
   if ed['path']==p:assert s.count(ed['new'])==1;s=s.replace(ed['new'],ed['old'],1)
 return s
def prepare(out,mode):
 rows=[e for e in PLAN['changes']if e['batch']==5];require_fixed([e['key']for e in rows]);out.mkdir(parents=True,exist_ok=True);parts=[]
 for i,e in enumerate(rows):
  s=selected(e,mode);case=e['case'];prefix='struct Fifth'+str(i)+':Base{using Base::Base;'
  if case=='copy':
   ed=e['edits'][0];s=ed['old'if mode=='baseline'else'new'];parts.append(prefix+'void Core(uint32 id){switch(id){'+s+'default:break;}}};')
   s=e['edits'][1]['old'if mode=='baseline'else'new'];parts.append(s+';');continue
  if case=='figment':body=block(s,r'void OnCreatureCreate_sha_of_doubt\(')
  elif case=='dred':
   s=block(s,r'struct boss_king_dredAI\b');ctor=block(s,r'boss_king_dredAI\(Creature\* creature\)');ctor=ctor.replace('boss_king_dredAI(','Fifth2(');field=re.search(r'uint(?:8|32) raptorsKilled[^;]*;',s)[0];body=ctor+field+''.join(block(s,r'(?:void|uint32) '+m+r'\(')for m in('Reset','DoAction','GetData'));prefix='struct Fifth2:BossAI{'
  elif case=='millificent':
   s=block(s,r'struct boss_millificent_manastormAI\b');ctor=block(s,r'boss_millificent_manastormAI\(Creature\* creature\)');ctor=ctor.replace('boss_millificent_manastormAI(','Fifth3(');fields=s[s.index('bool enrage;'):s.index('void Initialize()')];body=ctor+fields+''.join(block(s,r'(?:void|uint32) '+m+r'\(')for m in('Reset','ReceiveEmote','GetData'));prefix='struct Fifth3:BossAI{'
  elif case=='salsa':body=block(s,r'void DamageTaken\(Unit\* done_by')
  elif case=='tsunami':
   start=s.index('            if (m_uiFlameTsunamiTimer <= uiDiff)');end=s.index('            // flame breath',start);body='void Wave(uint32 uiDiff){'+s[start:end]+'}'
  elif case=='disks':
   s=block(s,r'struct boss_malygosAI\b');start=s.index('                    case EVENT_FLY_OUT_OF_PLATFORM:');end=s.index('                    case EVENT_PATHING_AROUND_PLATFORM:',start);body='void Reinforcements(uint32 id){switch(id){'+s[start:end]+'default:break;}}'
  elif case=='pit':
   s=block(s,r'struct instance_pit_of_saron_InstanceScript\b');body=block(s,r'bool SetBossState\(')
  elif case=='halfus':
   s=block(s,r'struct boss_halfus_wyrmbreakerAI\b');body=block(s,r'void Reset\(')
  elif case=='orb':
   s=block(s,r'struct npc_static_orbAI\b');body=block(s,r'void UpdateAI\(')
  else:raise AssertionError(case)
  parts.append(prefix+body+'};')
 text='\n'.join(parts)
 excluded={'NULL','AI','TYPEID_PLAYER','IN_MILLISECONDS','UNIT_FIELD_VIRTUAL_ITEMS','EQUIPMENT_SLOT_MAINHAND','EQUIPMENT_SLOT_OFFHAND','EQUIPMENT_SLOT_RANGED','INVENTORY_SLOT_BAG_0','SPELL_EFFECT_DUMMY','SPELL_EFFECT_SCRIPT_EFFECT','EFFECT_0','GO_STATE_READY','ALLIANCE','DONE','IN_PROGRESS','UNIT_STATE_STUNNED','PET_FOLLOW_DIST','MAX_MELEE_HOVER_DISK_SPAWNPOINTS'}
 labels=set(re.findall(r'\b[A-Z][A-Z_0-9]+\b',text))-excluded
 (out/'Selected.inc').write_text('enum FifthLabels{'+','.join(v+'='+str(100+i)for i,v in enumerate(sorted(labels)))+'};\n'+text,encoding='utf8',errors='surrogateescape')
 (out/'cases.json').write_text(json.dumps([e['key']for e in rows]));print('Extracted exact fifth-group production methods and wave/dispatch decision scopes;external endpoints controlled')
