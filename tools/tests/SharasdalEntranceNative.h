#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "LFGMgr.h"
#include "AreaTriggerData.h"
#include "MapManager.h"
#include "DB2Store.h"
#include "DB2LoadInfo.h"
#include <sstream>
struct SharasdalEntryTag{using Type=ScenarioResume Player::*;friend Type Access(SharasdalEntryTag);};
template struct FixtureAccess<SharasdalEntryTag,&Player::m_scenarioResume>;
static void Shaman(Player& p,unsigned race){p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_SHAMAN);p.setFactionForRace(race);p.SetQuestStatus(40341,QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,40341);}
#ifdef SHARASDAL_LEGACY_LOGIN_BASELINE
#include "SharasdalPinnedPrepareNative.h"
#endif
static void LoadSharasdalEntranceData(){
#ifndef SHARASDAL_ENTRANCE_LIVE
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("lfg_entrances","1");
#ifdef SHARASDAL_LEGACY_LOGIN_BASELINE
 WorldDatabase.DirectExecute("DELETE FROM lfg_entrances WHERE dungeonId=1229");WorldDatabase.DirectExecute("INSERT INTO lfg_entrances VALUES(1229,NULL,836.16,1035.4,48.61,1.7935)");
#else
 std::string migration=
#include "SharasdalEntranceMigration.inc"
 ;std::istringstream lines(migration);std::string line,sql;
 while(std::getline(lines,line)){auto first=line.find_first_not_of(" \t\r");if(first==std::string::npos||line.compare(first,2,"--")==0)continue;sql+=line+'\n';}
 std::string statement;bool quote=false;for(char c:sql){if(c=='\'')quote=!quote;if(c==';'&&!quote){WorldDatabase.DirectExecute(statement.c_str());statement.clear();}else statement+=c;}
 auto applied=WorldDatabase.Query("SELECT COALESCE(@sharasdal_resume_safe,0)");Require(applied&&applied->Fetch()[0].GetBool(),"Native TEMP migration refused the original graph");
#endif
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
#endif
 sSpellMgr->LoadSpellTargetPositions();
}
