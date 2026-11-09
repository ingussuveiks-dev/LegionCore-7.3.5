from pathlib import Path
import json,subprocess,sys,re
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);case=sys.argv[2];baseline=len(sys.argv)>3 and sys.argv[3]=='baseline'
def source(path):return subprocess.check_output(['git','-C',str(r),'show','efd389974:'+path]).decode('utf8')if baseline else(r/path).read_text(encoding='utf-8-sig')
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
dump=source('src/server/game/Tools/PlayerDump.cpp');header=source('src/server/game/Tools/PlayerDump.h');player=source('src/server/game/Entities/Player/Player.cpp')
low=dump[dump.index('static bool FindTokNth'):dump.index('std::string CreateDumpString')]
# Keep only numeric/string transformation helpers, never database import entrypoints.
(out/'DumpLow.inc').write_text(low,encoding='utf8')
(out/'DumpStores.inc').write_text(block(dump,'void StoreGUID(QueryResult result, uint32 field,')+'\n'+block(dump,'void StoreGUID(QueryResult result, uint32 data,'),encoding='utf8')
selected=''
if case=='Artifact':selected=block(source('src/server/scripts/Spells/spell_quest.cpp'),'class spell_q_artifact_trait_unlock :')+';'
elif case=='Chain':
 s=source('src/server/game/Spells/Spell.cpp');a=s.index('            uint'+('32'if baseline else'64')+' maxHPDeficit = 0;');b=s.index('\n        }\n        else',a);selected='WorldObject* SelectChain(Unit*m_caster,Unit*nextTarget,std::list<WorldObject*>&tempTargets,float jumpRadius){WorldObject*foundItr=nullptr;'+s[a:b]+'\nreturn foundItr;}'
elif case=='Regen':selected=block(player,'void Player::RegenerateHealth()')
elif case=='Resurrection':
 a=player.index('void Player::ResurrectPlayer(');selected='void Player::RestoreProbe(float restore_percent){'+block(player[a:],'    if (restore_percent > 0.0f)')+'}'
elif case=='Ancestral':selected=block(source('src/server/scripts/Spells/spell_shaman.cpp'),'class spell_sha_ancestral_protection :')+';'
elif case=='Betrug':
 s=source('src/server/scripts/Legion/VioletHoldLegion/boss_fel_lord_betrug.cpp');selected=block(s,' enum Spells')+';\n'+s[s.index('Position const jumpPos[4]'):s.index('//102446')]+block(s,'class spell_betrug_mighty_smash :')+';'
elif case=='Evade':
 s=source('src/server/scripts/Northrend/zone_zuldrak.cpp');s=s[s.index('class npc_orinoko_tuskbreaker :'):]
 selected=block(s,'        void EnterEvadeMode() override').replace('void EnterEvadeMode() override','void EvadeProbe::EnterEvadeMode()')+'\n'+block(s,'        void JustDied(Unit* killer) override').replace('void JustDied(Unit* killer) override','void EvadeProbe::JustDied(Unit* killer)')
elif case=='DumpIDs':
 selected=block(header,'class PlayerDumpWriter :')+';'if False else ''
 selected+='using WriterGUIDs='+re.search(r'typedef (std::set<[^>]+>) GUIDs;',header)[1]+';\n'
 line=next(x.strip()for x in dump.splitlines()if 'snprintf(newguid,' in x);size=re.search(r'char newguid\[(\d+)\]',dump)[1]
 selected+='std::string FormatGuid(uint64 guid){char newguid['+size+'];'+line+'return newguid;}\n'
 line=next(x.strip()for x in dump.splitlines()if 'snprintf(newItemId,' in x);size=re.search(r'char newItemId\[(\d+)\]',dump)[1]
 selected+='std::string FormatItem(uint64 newItemIdNum){char newItemId['+size+'];'+line+'return newItemId;}\n'
elif case=='DumpPets':
 selected=block(header,'enum DumpTableType')+';\n'
 if not baseline:selected+=block(dump,'static bool ChangePetReference(')+'\n'
 a=dump.index('            case DTT_PET:',dump.index('DumpReturn PlayerDumpReader::LoadDump('));b=dump.index('            case DTT_CHAR_VS_FOLLOW:',a)
 selected+='struct PetFrame{std::map<uint32,uint32>petids;char newguid[21],newpetid[20],currpetid[20],lastpetid[20]{};PetFrame(){snprintf(newguid,sizeof(newguid),UI64FMTD,uint64(1099511627793));'
 if baseline:selected+='snprintf(newpetid,sizeof(newpetid),"%u",sObjectMgr->GeneratePetNumber());'
 selected+='}bool Apply(std::string&line,bool declined=false,bool child=false){auto type=child?DTT_PET_TABLE:'
 selected+=('(declined?DTT_PET_DECLINED_NAME:DTT_PET)'if not baseline else'DTT_PET')+';switch(type){'+dump[a:b]+'}return true;}};'
elif case!='DumpParse':raise ValueError(case)
(out/'Selected.inc').write_text(selected,encoding='utf8')
