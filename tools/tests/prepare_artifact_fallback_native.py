"""Exact artifact plan, shadow bootstrap and production text builder for native checks."""
from pathlib import Path
import re,sys
from artifact_fallback_repairs import load,validate,filename,setup,ROOT
p=load();validate(p);out=Path(sys.argv[1]);sql=Path(sys.argv[2]);rows=p['repairs']
def quoted(s):return '"'+s+'"'
def cpp_sql(text):
 text='\n'.join(x for x in text.splitlines() if not x.startswith('--'))
 statements=[s for s in re.split(r";(?=(?:[^']*'[^']*')*[^']*$)",text) if s.strip()]
 return ','.join('std::string(R"AF('+s[:15000]+')AF")'+''.join('+R"AF('+s[i:i+15000]+')AF"' for i in range(15000,len(s),15000)) for s in statements)
lines=['struct TextRow {uint32 entry,group,id;char const* before;char const* after;uint32 type,lang,emote,duration,sound,broadcast,minTimer,maxTimer,spell;float probability;};']
lines.append('static TextRow const TextRows[]={'+','.join('{'+','.join([str(x['key'][k]) for k in ('CreatureID','GroupID','ID')]+[quoted(x['before_hex']),quoted(x['after_hex'])]+[str(x['values'][k]) for k in ('Type','Language','Emote','Duration','Sound','BroadcastTextID','MinTimer','MaxTimer','SpellID')]+[str(float(x['values']['Probability']))+'f'])+'}' for x in rows if x['table']=='creature_text')+'};')
lines.append('struct MenuRow {uint32 menu,id,npc,broadcast,boxBroadcast,actionMenu,poi,money;bool coded;char const* box;char const* before;char const* after;};')
lines.append('static MenuRow const MenuRows[]={'+','.join('{'+','.join([str(x['values'][k]) for k in ('MenuID','OptionID','OptionNpc','OptionBroadcastTextID','BoxBroadcastTextID','ActionMenuID','ActionPoiID','BoxMoney')]+['true' if x['values']['BoxCoded'] else 'false',quoted(x['values']['BoxText'].encode().hex().upper()),quoted(x['before_hex']),quoted(x['after_hex'])])+'}' for x in rows if x['table']=='gossip_menu_option')+'};')
lines.append('static std::string const ShadowSetup[]={'+cpp_sql(setup(p))+'};')
lines.append('static std::string const Migrations[]={'+cpp_sql('\n'.join((sql/filename(n)).read_text() for n in range(1,11)))+'};')
(out/'ArtifactFallbackRows.inc').write_text('\n'.join(lines),encoding='utf-8')
source=(ROOT/'src/server/game/Texts/CreatureTextMgr.cpp').read_text(encoding='utf-8');start=source.index('class CreatureTextBuilder\n');i=source.index('{',start);depth=1;j=i+1
while depth:depth+=(source[j]=='{')-(source[j]=='}');j+=1
(out/'NativeCreatureTextBuilder.inc').write_text(source[start:j]+';',encoding='utf-8')
