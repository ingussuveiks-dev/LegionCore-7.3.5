from pathlib import Path
import json
def generate(out):
    r=Path.cwd();d=json.loads((r/'tools/tests/FifthFifty20261114Repairs.json').read_text(encoding='utf-8'));parts=['int main(){try{\n']
    def add(row,body):
     key=row['case'];fn='Case'+key+'::Selected::'+row['method'];parts.append('#ifdef REPAIR_'+key.upper()+'\n{ activeCase = '+json.dumps(key)+'; auto fn = '+fn+'; '+body+'\n}\n#endif\n')
    prefix={
    'NpcMoveExactGuid':('7','7x','0','18446744073709551616','7', '',''),
    'NpcMovementExactGuid':('4294967303 way NODEL','7x way NODEL','0 way NODEL','18446744073709551616 way NODEL','4294967303','',''),
    'NpcActivationFullGuid':('4294967303','7x','0','1099511627776','4294967303','',''),
    'ListCreatureAtomicRequest':('7 4294967295','7x 2','7 2x','7 4294967296','7','',''),
    'ListItemAtomicRequest':('7 4294967295','7x 2','7 2x','7 4294967296','7','',''),
    'ListObjectAtomicRequest':('7 4294967295','7x 2','7 2x','7 4294967296','7','',''),
    'CreatureReloadAtomicEntries':('7 8','7x','7 8x','4294967296','7','',''),
    'PlayerIpSearchExactLimit':('192.0.2.1 -1','192.0.2.1 7x','192.0.2.1 -2147483649','192.0.2.1 2147483648','0','',''),
    'PlayerEmailSearchExactLimit':('test@example.invalid -1','test@example.invalid 7x','test@example.invalid -2147483649','test@example.invalid 2147483648','0','',''),
    'DeletedCharacterRetentionExactDays':('2147483647','7x','-1','2147483648','2147483647','',''),
    'DeletedCharacterRestoreExactAccount':('Controlled Newname 4294967295','Controlled Newname 7x','Controlled Newname -1','Controlled Newname 4294967296','4294967295','',''),
    'CharacterDumpImportFullKeys':('no-real-file 1 Controlled 4294967303','no-real-file 1x','no-real-file 1 Controlled 7x','no-real-file 1 Controlled 1099511627776','1','',''),
    'CharacterDumpExportExactCounter':('no-real-file 4294967303','no-real-file 18446744073709551616','no-real-file 1099511627776','no-real-file 999999999999999999999999999','4294967303','',''),
    'CharacterRenameHistoryFullGuid':('4294967303','18446744073709551616','1099511627776','99999999999999999999999','4294967303','',''),
    'MailListingExactPlayerCounter':('4294967303','7x','-1','1099511627776','4294967303','',''),
    'PlayerInfoExactCounter':('4294967303','7x','-1','1099511627776','4294967303','',''),
    'WaypointDisplayExactPathAndSpawns':('on 4294967295','on 7x','on -1','on 4294967296','4294967295','',''),
    'ArenaQueueNativeBracketAdmission':('1','1x','-1','256','3','',''),
    'DebugGroupBoundedRemainingMembers':('40','7x','-1','41','40','',''),
    'LfgListingNativeStatusWidth':('255','7x','0','256','255','',''),
    }
    for row in d['changes']:
     key=row['case']
     if key.startswith('Appearance'):
      field='m_vis'+key[len('Appearance'):-len('ExactItem')]
      slot={'Mainhand':'MAINHAND','Head':'HEAD','Shoulders':'SHOULDERS','Chest':'CHEST','Waist':'WAIST','Legs':'LEGS','Feet':'FEET','Wrists':'WRISTS','Hands':'HANDS','Back':'BACK','Offhand':'OFFHAND','Ranged':'RANGED','Tabard':'TABARD','Shirt':'BODY'}[key[len('Appearance'):-len('ExactItem')]]
      add(row,'''for(auto bad:{"7x","-1","4294967296","999999999999999999999","+","--7","0x7","7.0"}){Context c;Require(!Run(fn,c.handler,bad)&&!c.player.m_vis&&!c.player.modelCalls,"Malformed appearance changed model/allocated visual");}
    for(auto good:{"7","+7","0","4294967295","|cffffffff|Hitem:7|h[Controlled item]|h|r"}){Context c;Require(Run(fn,c.handler,good)&&c.player.modelCalls==1&&c.player.slot==EQUIPMENT_SLOT_'''+slot+'''&&c.player.m_vis->'''+field+'''==c.player.item,"Appearance item/slot/reset/link lost");}
    {Context c;c.player.modelAccept=false;Require(!Run(fn,c.handler,"7")&&c.player.m_vis->'''+field+'''==0,"Failed native model update persisted appearance");}
    {Context c;c.handler.session=nullptr;Require(!Run(fn,c.handler,"7"),"Missing appearance session");}
    {Context c;c.session.player=nullptr;Require(!Run(fn,c.handler,"7"),"Missing appearance player");}''')
     elif key in prefix and key!='NpcActivationFullGuid':
      good,bad1,bad2,bad3,expected,_,_=prefix[key]
      add(row,'Context c; '+('c.handler.creature=nullptr; 'if key=='NpcMoveExactGuid'else'')+'Require(Run(fn,c.handler,'+json.dumps(good)+')&&c.handler.reached==1&&c.handler.observed=='+expected+'ULL,"Native valid admission/counter lost"); for(auto bad:{'+','.join(json.dumps(s)for s in [bad1,bad2,bad3])+ '}){Context b;'+('b.handler.creature=nullptr;'if key=='NpcMoveExactGuid'else'')+'Require(!Run(fn,b.handler,bad)&&!b.handler.reached,"Malformed admission reached service");}')
     elif key=='NpcPassengerFailedCreationCleanup':
      add(row,'''for(auto bad:{"7x","7 1x","4294967296","7 2147483648","0","8"}){ResetSpawns();Context c;Require(!Run(fn,c.handler,bad)&&!creates&&!saves&&objectManager.data.empty(),"Invalid NPC request had effects");}
    for(bool fail:{false,true}){ResetSpawns();Context c;Transport transport;c.player.transport=&transport;transport.accepted=!fail;objectManager.data[9].id=99;bool ok=Run(fn,c.handler,"7 -1");Require(ok==!fail&&transport.calls==1,"Transport creation receipt lost");Require(objectManager.data.count(9)==1,"Unrelated spawn removed");Require(objectManager.data.count((1ULL<<32)+7)==(fail?0:1),"Failed spawn data leaked or success deleted");Require(objectManager.deletes==(fail?1:0)&&saves==(fail?0:1)&&objectManager.grids==(fail?0:1),"Failure saved/gridded or wrong cleanup");if(!fail){auto data=objectManager.data.at((1ULL<<32)+7);Require(data.posX==5&&data.posY==6&&data.posZ==7&&data.orientation==1&&data.mapid==9&&data.phaseMask==2,"Transport-local pose/phase/save map changed");} }
    for(int path=0;path<3;++path){ResetSpawns();Context c;createAccept=path!=1;loadAccept=path!=2;bool ok=Run(fn,c.handler,"|cffffffff|Hcreature_entry:7|h[NPC]|h|r 0");Require(ok==(path==0),"Native ordinary create/load result lost");if(path!=1)Require(loadedGuid==(1ULL<<32)+7,"Spawn reload truncated native GUID");}ResetSpawns();''')
     elif key=='NpcActivationFullGuid':add(row,'''for(auto bad:{"7x","0","1099511627776","18446744073709551616"}){Context c;WorldDatabase={};Require(!Run(fn,c.handler,bad)&&!WorldDatabase.queries&&!c.npc.active,"Malformed activation mutated actor");}Context c;WorldDatabase={};c.npc.liveGuid=19;Require(Run(fn,c.handler,"")&&c.npc.active&&WorldDatabase.rawValue==4294967303LL,"Activation persisted runtime/low32 alias");Require(Run(fn,c.handler,"")&&!c.npc.active,"Activation toggle lost");WorldDatabase={};c.npc.db=0;Require(Run(fn,c.handler,"")&&WorldDatabase.queries==0,"Unsaved actor wrote a persistent alias");c.npc.pet=true;Require(!Run(fn,c.handler,""),"Pet activation admitted");''')
     elif key=='SpellCheatMissingSession':add(row,'''Context c;c.handler.session=nullptr;Require(!Run(fn,c.handler,"on"),"No-session cheat accepted");c.handler.session=&c.session;c.session.player=nullptr;Require(!Run(fn,c.handler,"on"),"Missing player accepted");c.session.player=&c.player;Require(Run(fn,c.handler,"on")&&c.player.flags==(CHEAT_CASTTIME|CHEAT_COOLDOWN|CHEAT_POWER),"Native enable flags lost");Require(Run(fn,c.handler,"off")&&!c.player.flags,"Native disable flags lost");Require(!Run(fn,c.handler,"invalid"),"Unknown cheat accepted");''')
     elif key=='PetBattleFailureSafeSelection':add(row,'''for(auto bad:{"7x","-1","256","999999999999999999999"}){Context c;Require(!Run(fn,c.handler,bad)&&!c.session.responses,"Invalid pet response sent");}Context c;c.handler.target=nullptr;Require(!Run(fn,c.handler,"7"),"Missing object accepted");c.handler.unit=&c.npc;Require(!Run(fn,c.handler,"7"),"Nonplayer object accepted");c.handler.unit=&c.player;Require(Run(fn,c.handler,"255")&&c.session.responses==1&&c.session.reason==255,"Native byte response lost");''')
     elif key.startswith('AuraDuration')or key.startswith('AuraCharges'):
      charges=key.startswith('AuraCharges');bad=['7x','2147483648','-2147483649']if not charges else['7x','256','-1']
      val='charges'if charges else'duration';maximum='255'if charges else'2147483647'
      add(row,'for(auto bad:{'+','.join(json.dumps('123 '+s)for s in bad)+'}){Context c;Require(!Run(fn,c.handler,bad)&&!c.player.removals&&!c.player.aura.changes,"Malformed aura request changed state");} Context c;Require(Run(fn,c.handler,"123 '+maximum+'")&&c.player.aura.'+val+'=='+maximum+',"Native aura maximum lost");Require(Run(fn,c.handler,"123 0")&&c.player.removals==1,"Zero-removal lost");Require(!Run(fn,c.handler,"123 1"),"Missing aura accepted");c.handler.target=nullptr;Require(!Run(fn,c.handler,"123 1"),"Missing unit accepted");')
     elif key=='QuestFailureResponseExactCode':add(row,'''for(auto bad:{"7x","-1","4294967296",""}){Context c;Require(!Run(fn,c.handler,bad)&&!c.player.packets,"Malformed quest failure sent");}Context c;Require(Run(fn,c.handler,"4294967295")&&c.player.questCode==4294967295,"Native response width lost");''')
     elif key=='ItemSpecializationMissingCatalog':add(row,'''for(auto bad:{"7x","0","4294967296","8"}){Context c;Require(!Run(fn,c.handler,bad),"Invalid item catalog input accepted");}Context c;itemCatalog.present=false;Require(!Run(fn,c.handler,"7"),"Missing native catalog accepted");itemCatalog.present=true;Require(Run(fn,c.handler,"7"),"Native item catalog path lost");''')
     elif key=='DeletedCharacterSearchFullGuid':
      # Different signature: exported helper is executed directly against controlled query endpoints.
      parts.append('#ifdef REPAIR_'+key.upper()+'''\n{DeletedInfoList found;CharacterDatabase={};++cases;Require(Case'''+key+'''::Selected::GetDeletedCharacterInfoList(found,"4294967303")&&CharacterDatabase.last.integers[0]==4294967303ULL,"Deleted search counter truncated");for(auto bad:{"1099511627776","18446744073709551616"}){CharacterDatabase={};++cases;Require(!Case'''+key+'''::Selected::GetDeletedCharacterInfoList(found,bad)&&!CharacterDatabase.queries,"Overflowing deleted counter queried");}++cases;Require(Case'''+key+'''::Selected::GetDeletedCharacterInfoList(found,"Controlled"),"Native name search lost");}\n#endif\n''')
     elif key=='GameObjectTargetExactIdOrName':add(row,'''for(auto bad:{"4294967296","18446744073709551616"}){Context c;Require(!Run(fn,c.handler,bad)&&!c.handler.reached,"Overflow object ID queried");}for(auto name:{"7x","Home","0"}){Context c;Require(Run(fn,c.handler,name)&&c.handler.observed==0,"Full object name fallback lost");}Context c;Require(Run(fn,c.handler,"4294967295")&&c.handler.observed==4294967295,"Native GO ID width lost");''')
     elif key=='SpellFailureNative735Packet':add(row,'''for(auto bad:{"7x","-1","290","7 1x","7 1 2x","7 2147483648"}){Context c;Require(!Run(fn,c.handler,bad)&&!c.player.packets,"Malformed spell failure sent");}Context c;Require(Run(fn,c.handler,"289 -2147483648 2147483647")&&c.player.lastPacket.reason==289&&c.player.lastPacket.a==(-2147483647-1)&&c.player.lastPacket.b==2147483647&&c.player.lastPacket.spell==133,"Native independent arguments lost");Require(Run(fn,c.handler,"0")&&c.player.lastPacket.a==-1&&c.player.lastPacket.b==-1,"Native optional defaults lost");''')
     elif key=='LootRecipientNativeGuidFormatting':add(row,'''Context c;c.handler.creature=nullptr;Require(!Run(fn,c.handler,""),"Missing loot actor accepted");c.handler.creature=&c.npc;Require(Run(fn,c.handler,"")&&c.handler.lastText.find("GUID 4294967303, DB GUID 4294967303")!=std::string::npos,"Native high-counter loot actor output truncated");''')
     elif key=='WaypointMutationFullSpawnCounter':add(row,'''Context c;WorldDatabase={};WorldDatabase.hasRow=true;c.npc.liveGuid=19;Require(Run(fn,c.handler,"del")&&c.handler.observed==4294967303ULL&&WorldDatabase.last.integers[0]==4294967303ULL,"Selected waypoint DB counter truncated/used runtime GUID");WorldDatabase={};WorldDatabase.rows[WORLD_SEL_WAYPOINT_DATA_BY_POS]=true;c.handler.reached=0;Require(Run(fn,c.handler,"del")&&c.handler.reached==1&&c.handler.observed==4294967303ULL&&WorldDatabase.queries==2,"Coordinate fallback result lost outside branch");WorldDatabase={};c.handler.reached=0;Require(Run(fn,c.handler,"del")&&!c.handler.reached,"Missing waypoint fallback reached traversal");c.npc.entry=7;Require(!Run(fn,c.handler,"del"),"Non-waypoint selected actor accepted");''')
     elif key=='WaypointEventTypedMutation':add(row,'''for(auto bad:{"del","del 7x","add 4294967296","mod 7 posx nan","mod 7 posy inf","mod 7 posz 1e100","mod 7 orientation 2x","mod 7 dataint 2147483648","mod 7 command -1","mod 7 datalong 7x","mod 7 setid 7x"}){WorldDatabase={};Context c;Require(!Run(fn,c.handler,bad)&&WorldDatabase.writes.empty()&&WorldDatabase.queries==0,"Invalid waypoint mutation reached DB");}
    {WorldDatabase={};Context c;Require(!Run(fn,c.handler,"add")&&WorldDatabase.writes.empty(),"Failed MAX query dereferenced/wrote");}
    {WorldDatabase={};WorldDatabase.hasRow=true;WorldDatabase.first=4294967295;Context c;Require(!Run(fn,c.handler,"add")&&WorldDatabase.writes.empty(),"Waypoint ordinal wrapped");}
    {WorldDatabase={};WorldDatabase.hasRow=true;WorldDatabase.first=0;Context c;Require(Run(fn,c.handler,"add")&&WorldDatabase.writes.size()==1&&WorldDatabase.writes.back().integers[0]==1,"Empty MAX/first ordinal lost");}
    {WorldDatabase={};Context c;Require(Run(fn,c.handler,"add 7")&&WorldDatabase.writes.size()==1&&WorldDatabase.writes.back().integers[0]==7,"Explicit new event lost");}
    {WorldDatabase={};WorldDatabase.hasRow=true;Context c;Require(Run(fn,c.handler,"mod 7 dataint -2147483648")&&WorldDatabase.rawValue==(-2147483647-1)&&WorldDatabase.sql.find("%i")!=std::string::npos,"Signed native dataint lost");}
    {WorldDatabase={};WorldDatabase.hasRow=true;Context c;Require(Run(fn,c.handler,"mod 7 posx -12.5")&&WorldDatabase.writes.back().floats[0]==-12.5f,"Finite negative coordinate lost");}''')
     elif key=='LinkedWorldObjectExactCounter':
      parts.append('#ifdef REPAIR_'+key.upper()+'''\n{Context c;objectManager.data[4294967303ULL].id=7;for(auto type:{"creature","gameobject"}){for(auto value:{"7x","-1","1099511627776","18446744073709551616"}){std::string s=std::string("|cffffffff|H")+type+":"+value+"|h[Test]|h|r";++cases;Require(!c.handler.extractGuidFromLink(s.data()),"Malformed link aliased world object");}std::string s=std::string("|cffffffff|H")+type+":4294967303|h[Test]|h|r";++cases;Require(c.handler.extractGuidFromLink(s.data()).GetGUIDLow()==4294967303ULL,"High counter lost");}}\n#endif\n''')
     elif key=='TeleportLinkExactIdOrName':
      parts.append('#ifdef REPAIR_'+key.upper()+'''\n{Context c;for(auto name:{"7th","Home","0"}){std::string text=name;++cases;Require(c.handler.extractGameTeleFromLink(text.data())&&objectManager.teleName==name,"Complete teleport name lost/aliased");}for(auto invalid:{"4294967296","99999999999999999999"}){std::string text=invalid;++cases;Require(!c.handler.extractGameTeleFromLink(text.data()),"Overflow teleport ID aliased");}std::string text="7";++cases;Require(c.handler.extractGameTeleFromLink(text.data())&&objectManager.teleId==7,"Native numeric teleport lost");}\n#endif\n''')
     elif key=='TypedCommandArgumentsBoundsAndValues':
      parts.append('#ifdef REPAIR_'+key.upper()+'''\n{Context c;CommandArgs args;args._handler=&c.handler;for(auto bad:{"7x","4294967296","-1","7 8"}){std::string text=bad;args._charArgs=text.data();++cases;args.Initialize({CommandArgs::ARG_UINT});Require(!args._validArgs,"Invalid/extra typed argument admitted");}for(auto bad:{"nan","inf","1e100","2x"}){std::string text=bad;args._charArgs=text.data();++cases;args.Initialize({CommandArgs::ARG_FLOAT});Require(!args._validArgs,"Nonfinite typed argument admitted");}std::string text="4294967295 -2147483648 -12.5";args._charArgs=text.data();++cases;args.Initialize({CommandArgs::ARG_UINT,CommandArgs::ARG_INT,CommandArgs::ARG_FLOAT});Require(args._validArgs&&std::any_cast<uint32>(args._args[0])==4294967295&&std::any_cast<int32>(args._args[1])==(-2147483647-1)&&std::any_cast<float>(args._args[2])==-12.5f,"Native typed values lost");text="7";args._charArgs=text.data();++cases;args.Initialize({CommandArgs::ARG_UINT});Require(args._validArgs&&args._args.size()==1,"Reinitialize retained stale values");text="\\\"";args._charArgs=text.data();++cases;args.Initialize({CommandArgs::ARG_QUOTE_ENCLOSED_STRING});Require(!args._validArgs,"Null quoted string accepted");}\n#endif\n''')
     else:raise ValueError(key)
    parts.append(r'''
#ifdef REPAIR_DELETEDCHARACTERRETENTIONEXACTDAYS
{GameTime::now=1700000000;for(uint32 days:{0U,1U,30U}){CharacterDatabase={};Player::deletions=0;++cases;Player::DeleteOldCharacters(days);Require(CharacterDatabase.last.integers[0]==1700000000ULL-uint64(days)*DAY&&!Player::deletions,"Native deletion cutoff/width changed");}for(uint32 days:{50000U,2147483647U,4294967295U}){CharacterDatabase={};++cases;Player::DeleteOldCharacters(days);Require(!CharacterDatabase.queries,"Before-epoch retention wrapped into deletion query");}GameTime::now=-1;CharacterDatabase={};++cases;Player::DeleteOldCharacters(0);Require(!CharacterDatabase.queries,"Negative clock queried deletion cutoff");GameTime::now=1700000000;}
#endif
''')
    parts.append(r'''
#ifdef REPAIR_DEBUGGROUPBOUNDEDREMAININGMEMBERS
{for(uint32 members:{1U,5U,40U})for(uint8 request:{uint8(1),uint8(5),uint8(40)}){ControlledGroup group{members};++cases;Require(NativeGroupRemaining(request,&group)==(request>members?request-members:0),"Native remaining count underflowed");}}
#endif
''')
    parts.append(r'''
#ifdef COUNTERFACTUAL_DELETEDCHARACTERRETENTIONEXACTDAYS
{GameTime::now=1700000000;CharacterDatabase={};++cases;Player::DeleteOldCharacters(50000);Require(CharacterDatabase.queries==1&&CharacterDatabase.last.integers[0]==1674967296ULL,"Pinned old retention cutoff wrap did not reproduce");}
#endif
#ifdef COUNTERFACTUAL_APPEARANCEMAINHANDEXACTITEM
{Context c;Require(Run(CaseAppearanceMainhandExactItem::Selected::HandleCharDisplayMainhandCommand,c.handler,"7x")&&c.player.item==7,"Pinned old numeric alias did not reproduce");}
#endif
#ifdef COUNTERFACTUAL_NPCPASSENGERFAILEDCREATIONCLEANUP
{ResetSpawns();Context c;Transport transport;c.player.transport=&transport;Require(Run(CaseNpcPassengerFailedCreationCleanup::Selected::HandleNpcAddCommand,c.handler,"7x")&&saves==1,"Pinned old NPC entry alias did not reproduce");}
#endif
#ifdef COUNTERFACTUAL_WAYPOINTEVENTTYPEDMUTATION
{WorldDatabase={};WorldDatabase.hasRow=true;WorldDatabase.first=4294967295;Context c;Require(Run(CaseWaypointEventTypedMutation::Selected::HandleWpEventCommand,c.handler,"add")&&WorldDatabase.writes.back().integers[0]==0,"Pinned old waypoint ordinal wrap did not reproduce");}
#endif
#ifdef COUNTERFACTUAL_TYPEDCOMMANDARGUMENTSBOUNDSANDVALUES
{Context c;CommandArgs args;args._handler=&c.handler;std::string text="7x";args._charArgs=text.data();args.Initialize({CommandArgs::ARG_UINT});++cases;Require(args._validArgs&&std::any_cast<uint32>(args._args[0])==7,"Pinned old typed numeric alias did not reproduce");}
#endif
#ifdef COUNTERFACTUAL_LINKEDWORLDOBJECTEXACTCOUNTER
{Context c;objectManager.data[7].id=7;std::string text="|cffffffff|Hcreature:7x|h[Old alias]|h|r";++cases;Require(c.handler.extractGuidFromLink(text.data()).GetGUIDLow()==7,"Pinned old GUID link alias did not reproduce");}
#endif
''')
    parts.append('ResetSpawns();std::cout<<"PASS "<<cases<<" production-function cases;controlled endpoints,no live DB writes"<<std::endl;return 0;}catch(std::exception const&e){std::cerr<<activeCase<<": "<<e.what()<<std::endl;return 1;}}\n')
    (out/'Cases.inc').write_text(''.join(parts),encoding='utf-8')
