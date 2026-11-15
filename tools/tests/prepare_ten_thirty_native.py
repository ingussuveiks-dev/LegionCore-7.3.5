"""Generate native rows/SQL chunks from the exact tested plan and source builder."""
from pathlib import Path
import json,re,sys
from ten_thirty_text_repairs import load,filename,ROOT
from prepare_ten_thirty_text_tests import prior
out=Path(sys.argv[1]);sql=Path(sys.argv[2]);plan=load();rows=plan['repairs'];go=[x for x in rows if x['table']=='gameobject_template'];ct=[x for x in rows if x['table']=='creature_text']
lines=['struct NameRow {uint32 entry;char const* before;char const* after;};','static NameRow const NameRows[]={'+','.join('{'+str(x['key']['entry'])+',"'+x['before_hex']+'","'+x['after_hex']+'"}' for x in go)+'};','static char const* NameIds="'+','.join(str(x['key']['entry']) for x in go)+'";','static char const* NameMigrations[]={"DO 0"};']
lines.append('static NameRow const NameControls[]={'+','.join('{'+str(x['entry'])+',"'+x['before_hex']+'","'+x['after_hex']+'"}' for x in prior(plan))+'};')
lines.append('struct TextRow {uint32 entry,group,id;char const* before;char const* after;uint32 type,lang,emote,duration,sound,broadcast,minTimer,maxTimer,spell;float probability;};static TextRow const TextRows[]={'+','.join('{'+','.join([str(x['key'][k]) for k in ('CreatureID','GroupID','ID')]+['"'+x['before_hex']+'"','"'+x['after_hex']+'"']+[str(x['values'][k]) for k in ('Type','Language','Emote','Duration','Sound','BroadcastTextID','MinTimer','MaxTimer','SpellID')]+[str(float(x['values']['Probability']))+'f'])+'}' for x in ct)+'};')
statements=[]
for batch in range(1,11):
 text='\n'.join(x for x in (sql/filename(batch)).read_text(encoding='utf-8').splitlines() if not x.startswith('--'))
 statements.extend(s for s in re.split(r";(?=(?:[^']*'[^']*')*[^']*$)",text) if s.strip())
chunks=[]
for s in statements:
 parts=[s[i:i+16000] for i in range(0,len(s),16000)];chunks.append('std::string(R"TTXT('+parts[0]+')TTXT")'+''.join('+R"TTXT('+p+')TTXT"' for p in parts[1:]))
lines.append('static std::string const TextMigrations[]={'+','.join(chunks)+'};')
(out/'GameObjectNameRows.inc').write_text('\n'.join(lines),encoding='utf-8')
source=(ROOT/'src/server/game/Texts/CreatureTextMgr.cpp').read_text(encoding='utf-8');start=source.index('class CreatureTextBuilder\n');i=source.index('{',start);depth=1;j=i+1
while depth:depth+=(source[j]=='{')-(source[j]=='}');j+=1
(out/'NativeCreatureTextBuilder.inc').write_text(source[start:j]+';',encoding='utf-8')
