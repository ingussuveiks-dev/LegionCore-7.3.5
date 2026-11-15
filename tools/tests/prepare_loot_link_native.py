from pathlib import Path
import sys,re
from loot_link_repairs import load,ROOT,predicate
from prepare_loot_link_tests import setup
out=Path(sys.argv[1]);mig=Path(sys.argv[2]).read_text(encoding='utf-8');p=load()
def extract(source,start):
 a=source.index(start);i=source.index('{',a);depth=1;j=i+1
 while depth:depth+=(source[j]=='{')-(source[j]=='}');j+=1
 return source[a:j]
def chunks(s):return 'std::string('+')+std::string('.join('R"LLNK('+s[i:i+16000]+')LLNK"' for i in range(0,len(s),16000))+')'
def array(name,s):
 s='\n'.join(l for l in s.splitlines() if not l.startswith('--'));return 'static std::string const '+name+'[]={'+','.join(chunks(x) for x in re.split(r";(?=(?:[^']*'[^']*')*[^']*$)",s) if x.strip())+'};'
rows=p['repairs'];lines=[array('SetupSql',setup(p,'').split('SET SESSION TRANSACTION READ ONLY;')[0]),array('Migrations',mig),array('ResetSql','\n'.join(f"UPDATE creature_template g SET lootid=0 WHERE {predicate(r)};" for r in rows)),'static uint32 const Entries[]={'+','.join(str(r['entry']) for r in rows)+'};','static char const* EntryIds="'+','.join(str(r['entry']) for r in rows)+'";','static uint32 const LootRowCount='+str(sum(r['loot_rows'] for r in rows))+';']
base=(ROOT/'tools/tests/RockawayChannelNative.cpp').read_text(encoding='utf-8');lines.insert(0,base[:base.index('// The scheduler lookup')]+'\n#include "Pet.h"\n#include "WorldSession.h"\n#include "LootMgr.h"\n#include "CellImpl.h"\n');actor=extract(base,'struct Actor : Creature').replace('GetEntry() == 115133 ? static_cast<CreatureAI*>(new npc_rockaway_vision(this)) : new NullCreatureAI(this)','new NullCreatureAI(this)');lines.append(actor+';')
player=extract((ROOT/'tools/tests/RockawayOffensiveNative.cpp').read_text(encoding='utf-8'),'struct NativePlayer : Player');a=player.index('        // Controlled accepted-task state;');b=player.index('\n    }',a);player=player[:a]+player[b:];lines.append(player+';')
credit=(ROOT/'tools/tests/AltheaCreditNative.cpp').read_text(encoding='utf-8');lines.append(extract(credit,'struct CreditPet:Pet')+';');lines.append(extract(credit,'struct CreditGuardian:Guardian')+';')
(out/'NativeLootLink.inc').write_text('\n'.join(lines),encoding='utf-8')
