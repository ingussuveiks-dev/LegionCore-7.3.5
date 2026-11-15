from pathlib import Path
import sys,re
from next_text_repairs import load,ROOT,filename,match
from prepare_next_text_tests import setup
out=Path(sys.argv[1]);sql=Path(sys.argv[2]);p=load()
def chunks(s):
 return 'std::string('+ ')+std::string('.join('R"NTXT('+s[i:i+16000]+')NTXT"' for i in range(0,len(s),16000))+')'
def array(name,s):
 s='\n'.join(l for l in s.splitlines() if not l.startswith('--'));stmts=[x for x in re.split(r";(?=(?:[^']*'[^']*')*[^']*$)",s) if x.strip()]
 return 'static std::string const '+name+'[]={'+','.join(chunks(x) for x in stmts)+'};'
lines=['struct NameRow{uint32 entry;char const* before;char const* after;};static NameRow const NameRows[]={{0,"",""}};static char const* NameIds="0";static char const* NameMigrations[]={"DO 0"};']
lines += [array('SetupSql',setup(p,'').split('SET SESSION TRANSACTION READ ONLY;')[0]),array('Migrations','\n'.join((sql/filename(n)).read_text(encoding='utf-8') for n in range(1,10))),array('ResetSql','\n'.join(f"UPDATE `{r['table']}` g SET `{r['field']}`=CONVERT(0x{r['before_hex']} USING utf8mb4) WHERE {match(r,('after_hex',))};" for r in p['repairs']))]
lines+=['struct RestoreRow{char const* table;uint32 a,b;char const* before;char const* after;};static RestoreRow const RestoreRows[]={'+','.join('{"'+r['table']+'",'+str(list(r['key'].values())[0])+','+str(list(r['key'].values())[1] if len(r['key'])>1 else 0)+',"'+r['before_hex']+'","'+r['after_hex']+'"}' for r in p['repairs'])+'};']
lines+=['struct GroupRow{uint32 entry,group;};static GroupRow const GroupRows[]={'+','.join('{'+str(g['entry'])+','+str(g['group'])+'}' for g in p['groups'])+'};']
source=(ROOT/'src/server/game/Globals/QuestData.cpp').read_text(encoding='utf-8');start=source.index('QueryResult result = WorldDatabase.Query("SELECT "',source.index('void QuestDataStoreMgr::LoadQuests()'));end=source.index(');',start);query=''.join(re.findall(r'"([^"]*)"',source[start:end]));assert query.endswith(' FROM quest_template');lines.append('static std::string const QuestQuery='+chunks(query+' WHERE ID=')+';')
(out/'GameObjectNameRows.inc').write_text('\n'.join(lines),encoding='utf-8')
source=(ROOT/'src/server/game/Texts/CreatureTextMgr.cpp').read_text(encoding='utf-8');a=source.index('class CreatureTextBuilder\n');b=source.index('class PlayerTextBuilder',a);(out/'NativeCreatureTextBuilder.inc').write_text(source[a:b],encoding='utf-8')
