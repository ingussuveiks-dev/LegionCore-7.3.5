from pathlib import Path
import sys,re
from following_go_loot import load,filename,ROOT,predicate
from prepare_following_go_loot_tests import setup
out=Path(sys.argv[1]);sql_dir=Path(sys.argv[2]);p=load()
def extract(s,start):
 a=s.index(start);i=s.index('{',a);depth=1;j=i+1
 while depth:depth+=(s[j]=='{')-(s[j]=='}');j+=1
 return s[a:j]
def chunks(s):return 'std::string('+')+std::string('.join('R"FGOL('+s[i:i+16000]+')FGOL"' for i in range(0,len(s),16000))+')'
def array(name,s):
 s='\n'.join(l for l in s.splitlines() if not l.startswith('--'));return 'static std::string const '+name+'[]={'+','.join(chunks(x) for x in re.split(r";(?=(?:[^']*'[^']*')*[^']*$)",s) if x.strip())+'};'
base=(ROOT/'tools/tests/RockawayChannelNative.cpp').read_text(encoding='utf-8');lines=[base[:base.index('// The scheduler lookup')]+'\n#include "WorldSession.h"\n#include "LootMgr.h"\n#include "CellImpl.h"\n#include "GameObject.h"\n#include "GameObjectData.h"\n#include "Opcodes.h"\n']
player=extract((ROOT/'tools/tests/RockawayOffensiveNative.cpp').read_text(encoding='utf-8'),'struct NativePlayer : Player');a=player.index('        // Controlled accepted-task state;');b=player.index('\n    }',a);lines.append(player[:a]+player[b:]+';')
rows=p['repairs'];lines += [array('SetupSql',setup(p,'').split('SET SESSION TRANSACTION READ ONLY;')[0]),array('Migrations','\n'.join((sql_dir/filename(n)).read_text(encoding='utf-8') for n in range(1,11))),array('ResetSql','\n'.join(f"UPDATE gameobject_template g SET Data1=0 WHERE {predicate(r)};" for r in rows)),'static uint32 const Entries[]={'+','.join(str(r['entry']) for r in rows)+'};','static uint32 const LootRowCount='+str(sum(r['loot_rows'] for r in rows))+';']
trace=(ROOT/'tools/tests/RockawayOffensiveNative.cpp').read_text(encoding='utf-8');lines.append('#include <DbgHelp.h>\n#pragma comment(lib, "Dbghelp.lib")\n'+extract(trace,'static LONG WINAPI CrashTrace'))
(out/'NativeFollowingGoLoot.inc').write_text('\n'.join(lines),encoding='utf-8')
