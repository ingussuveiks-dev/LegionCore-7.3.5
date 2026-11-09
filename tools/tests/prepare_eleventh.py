from pathlib import Path
import json,re,subprocess,sys
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);plan=json.loads((r/'tools/tests/EleventhTenRepairs.json').read_text());parts=[];rows=[]
def source(path,base):return subprocess.check_output(['git','-C',str(r),'show','ff8303b40:'+path]).decode('utf8')if base else(r/path).read_text(encoding='utf-8-sig')
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
def setup(e,c,base):
 s=source(e['paths'][0],base);scope=block(s,re.search(r'(?:struct|class)\s+'+c['cls']+r'\s*:',s).group());method=block(scope,'void '+c['method']+'(')
 if e['key']=='Nightmare':assert 'DefaultEvents('+str(c['parameter'])+')'in scope
 # Select one contiguous resource block, preserving the exact original order.
 patterns=r'me->(?:SetPowerType\(POWER_ENERGY\)|SetMaxPower\(POWER_ENERGY, 100\)|SetPower\(POWER_ENERGY, (?:\d+|powerCount)\)|SetInt32Value\(UNIT_FIELD_(?:MAX_)?POWER, \d+\));'
 groups=list(re.finditer(r'(?:'+patterns+r'\s*)+',method))
 if e['key']=='Allies':groups=[m for m in groups if 'SetPower(POWER_ENERGY, '+str(c['start'])+');'in m.group()]
 if e['key']=='Ysondre'and not groups:body='' # Permit focused checks before unrelated commits are applied.
 else:assert len(groups)==1,(e['key'],c,groups);body=groups[0].group().strip()
 if e['key']=='Nightmare':body='uint8 powerCount='+str(c['parameter'])+';'+body
 if c.get('empty_spell'):assert 'DoCast(me, SPELL_EMPTY_ENERGY, true);'in method;body+='\nme->CastSpell(me, '+str(c['empty_spell'])+', true);'
 return body
for e in plan:
 if e['kind']=='power':
  for c in e['contexts']:
   idx=len(rows)
   for baseline in [True,False]:parts.append('static void '+('Old'if baseline else'Fixed')+str(idx)+'(Creature*me){'+setup(e,c,baseline)+'}')
   script=c['cls'][:-2]if c['cls'].endswith('AI')else c['cls'];rows.append('{"'+e['key']+'","'+script+'",'+str(c['npc'])+','+str(c['start'])+',&Old'+str(idx)+',&Fixed'+str(idx)+'}')
parts.append('struct EleventhSetup{char const*key;char const*script;uint32 npc,start;void(*old)(Creature*);void(*fixed)(Creature*);};static EleventhSetup eleventhSetups[]={'+','.join(rows)+'};')
e=next(e for e in plan if e['key']=='Announcer');s=source(e['paths'][0],False);scope=block(s,'struct npc_announcer_toc5AI :');method=block(scope,'void AnnounceChampion(');body=block(method,'if (champion->GetTypeId() == TYPEID_PLAYER)');parts.append('struct EleventhAnnounceBase:AnnounceBase{using AnnounceBase::AnnounceBase;void AnnounceChampion(Unit*champion){'+body+'}};')
for baseline in [True,False]:
 s=source(e['paths'][0],baseline);old=source(e['paths'][0],True);chunk=(e['edits'][0]['old']if baseline else e['edits'][0]['new']);assert chunk in s
 parts.append('struct '+('OldAnnounce'if baseline else'FixedAnnounce')+':EleventhAnnounceBase{using EleventhAnnounceBase::EleventhAnnounceBase;void Tick(){switch(2){'+chunk+'}}};')
(out/'EleventhSetups.inc').write_text('\n'.join(parts),encoding='utf8')
# Keep native/source declared conventions explicit and tied to the producer version.
m=re.search(r'EMOTE_ONESHOT_CHEER\s*=\s*(\d+)',(r/'src/server/game/Miscellaneous/SharedDefines.h').read_text(encoding='utf-8-sig'));assert m;(out/'EleventhConstants.inc').write_text('constexpr uint32 ORIGINAL_ANNOUNCER_EMOTE_CHEER='+m[1]+';\n')
