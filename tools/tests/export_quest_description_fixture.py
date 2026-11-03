"""Export actual loader projection and query-packet text selection, no guessed offsets."""
from pathlib import Path
import json,re,subprocess,sys
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;depth=1
 while depth:
  depth+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
def shape(s):return re.sub(r'\s','',re.sub(r'//[^\n]*|/\*.*?\*/','',s,flags=re.S))
path='src/server/game/Quests/QuestDef.cpp';current=(r/path).read_text();old=subprocess.check_output(['git','-C',str(r),'show','fcd85ecb4:'+path],text=True)
before='QuestDescription = questRecord[index++].GetString(); LogDescription = questRecord[index++].GetString();'
after='LogDescription = questRecord[index++].GetString(); QuestDescription = questRecord[index++].GetString();'
assert shape(current)==shape(old) or shape(current).replace(shape(after),shape(before),1)==shape(old),'Only two corresponding text assignments may swap'
load=block((r/'src/server/game/Globals/QuestData.cpp').read_text(),'void QuestDataStoreMgr::LoadQuests()')
a=load.index('WorldDatabase.Query(')+len('WorldDatabase.Query(');b=load.index(');',a)
query=''.join(json.loads(m.group()) for m in re.finditer(r'"(?:[^"\\]|\\.)*"',load[a:b]))
assert query.startswith('SELECT ') and query.endswith(' FROM quest_template')
columns=[s.strip().strip('`') for s in query[7:-len(' FROM quest_template')].split(',')]
names=['LogTitle','LogDescription','QuestDescription','AreaDescription','QuestCompletionLog','PortraitGiverText','PortraitGiverName','PortraitTurnInText','PortraitTurnInName']
start=columns.index('LogTitle');assert columns[start:start+9]==names
(out/'QuestTextSelect.inc').write_text('constexpr char QuestTextSelect[]='+json.dumps(query)+';\nconst std::vector<std::string> QuestTextColumns='+json.dumps(columns).replace('[','{').replace(']','}')+';\n')
source=block((r/'src/server/game/Entities/Creature/GossipDef.cpp').read_text(),'void PlayerMenu::SendQuestQueryResponse(')
a=source.index('    std::string questLogTitle');b=source.index('    WorldPackets::Quest::QueryQuestInfoResponse packet;')
assignments='\n'.join(line for line in source.splitlines() if re.match(r'\s*packet.Info.('+'|'.join(names)+r')\s*=',line))
assert len(assignments.splitlines())==9
(out/'QuestTextPacket.inc').write_text(source[a:b]+'\nPacket packet;\n'+assignments+'\n')
for filename,text in {
 'Expected':'\n'.join(f'expected.{n}=fields[index("{n}")].GetString();' for n in names),
 'Loaded':'\n'.join(f'loaded.{n}=quest.{n};' for n in names),
 'Translation':'\n'.join(f'translated.{n}.resize(1);translated.{n}[0]="translated-{n}";' for n in names),
 'Overlay':'\n'.join(f'overlaid.{n}="translated-{n}";' for n in names),
 'Check':'\n'.join(f'if(actual.{n}!=wanted.{n}){{Require(counterfactual&&actual.{n}==fallback.{other}&&wanted.{n}==fallback.{n},"Unexpected text-role/fallback mismatch");++mismatches;}}' if (other:={'LogDescription':'QuestDescription','QuestDescription':'LogDescription'}.get(n)) else f'Require(actual.{n}==wanted.{n},"Other text role changed");' for n in names)
}.items():(out/f'QuestText{filename}.inc').write_text(text+'\n')
(out/'source.json').write_text(json.dumps({'Columns':columns,'TextStart':start,'NativeConstructor':'Quest::Quest(Field*)','Packet':'PlayerMenu::SendQuestQueryResponse text selection and assignments only','Scope':'Other complete QuestDef.cpp bytes retained'},indent=2)+'\n')
print('PASS actual SQL column order/full-file scope and nine query-packet text assignments exported')
