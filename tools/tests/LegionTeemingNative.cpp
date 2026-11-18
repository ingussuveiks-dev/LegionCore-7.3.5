// Matching Teeming spawn data and actual Creature DB-load admission.
#define CHALLENGE_DEATH_SUPPORT_ONLY
#include "LegionChallengeDeathAffixesNative.cpp"

static void Metadata()
{
    auto groups=WorldDatabase.Query("SELECT map,COUNT(*) FROM creature WHERE isTeemingSpawn<>0 GROUP BY map ORDER BY map");
    if(groups)do{auto f=groups->Fetch();std::cout<<"META flagged map="<<f[0].GetUInt32()<<" count="<<f[1].GetUInt32()<<'\n';}while(groups->NextRow());
    for(uint32 id:{206151u,221772u})
    {
        auto info=sSpellMgr->GetSpellInfo(id);auto row=sSpellStore.LookupEntry(id);Require(info&&row,"Matching player affix spell absent");
        std::cout<<"META spell="<<id<<" name="<<row->Name->Str[LOCALE_enUS]<<" description="<<row->Description->Str[LOCALE_enUS]<<" duration="<<info->GetDuration(8)<<'\n';
    }
}

static CreatureData SelectSpawn()
{
    // Live data currently has no Teeming markers. Use one real ordinary
    // record with a controlled cache-only marker to exercise native admission;
    // this is not evidence for an original Teeming placement or a data repair.
    auto rows=WorldDatabase.Query("SELECT guid,id,map,position_x,position_y,position_z,orientation,phaseMask,spawnMask,spawntimesecs,spawndist FROM creature WHERE id=97043 AND map=1492 ORDER BY guid LIMIT 1");
    Require(rows&&rows->GetRowCount()==1,"Matching ordinary Maw spawn unavailable for controlled flag fixture");auto f=rows->Fetch();
    CreatureData data;data.guid=f[0].GetUInt64();data.id=f[1].GetUInt32();data.mapid=f[2].GetUInt16();
    data.posX=f[3].GetFloat();data.posY=f[4].GetFloat();data.posZ=f[5].GetFloat();data.orientation=f[6].GetFloat();
    data.phaseMask=f[7].GetUInt32();data.spawnMask=f[8].GetUInt64();data.spawntimesecs=f[9].GetUInt32();data.spawndist=f[10].GetFloat();data.isTeemingSpawn=true;
    std::cout<<"META selected guid="<<data.guid<<" entry="<<data.id<<" map="<<data.mapid<<" spawnMask="<<data.spawnMask<<'\n';
    Require(!sObjectMgr->GetCreatureData(data.guid),"Native test spawn key already published");sObjectMgr->NewOrExistCreatureData(data.guid)=data;return data;
}

static bool LoadSpawn(CreatureData const& data, Map& map)
{
    Creature actor;
    bool loaded=actor.LoadCreatureFromDB(data.guid,&map,false);
#ifndef TEEMING_BASELINE
    if(loaded)
    {
        Check(actor.GetDBTableGUIDLow()==data.guid&&actor.GetEntry()==data.id&&actor.GetMap()==&map,"Accepted spawn lost actual DB identity/map");
        Check(actor.GetPositionX()==data.posX&&actor.GetPositionY()==data.posY&&actor.GetPositionZ()==data.posZ,"Admission changed native DB pose");
        Check(actor.GetCreatureData()==sObjectMgr->GetCreatureData(data.guid),"Accepted spawn lost its actual cache record");
    }
#endif
    // Rejection precedes Object::_Create; no update fields exist to clean up.
    if(actor.GetMap()){actor.CombatStop(true);actor.CastStop();actor.RemoveAllAuras();actor.m_Events.KillAllEvents(true);actor.ResetMap();}
    return loaded;
}

int main(int argc,char**argv)
{
    try
    {
        InitializeDungeon(argc,argv,true);sObjectMgr->LoadItemTemplates();sObjectMgr->LoadItemTemplateAddon();sObjectMgr->LoadScenarioData();Metadata();auto data=SelectSpawn();
        unsigned id=0;
        for(Difficulty difficulty:{DIFFICULTY_NORMAL,DIFFICULTY_HEROIC,DIFFICULTY_MYTHIC_DUNGEON,DIFFICULTY_MYTHIC_KEYSTONE})
        for(unsigned profile=0;profile<
#ifdef TEEMING_BASELINE
            4
#else
            6
#endif
            ;++profile)
        {
            auto parent=std::make_unique<MapInstanced>(1492,300);auto map=std::make_unique<InstanceMap>(1492,300,9943000+id++,difficulty,parent.get());map->CreateInstanceData(nullptr);
            auto player=std::make_unique<LifecyclePlayer>(*map,801,Position{2934.49f,715.757f,554.286f,1.55255f});
            auto route=sMapChallengeModeStore.LookupEntry(208);Require(route,"Matching Maw route absent");auto key=player->Key(*route);player->m_challengeKeyInfo.Affix=(profile==2||profile>=4)?Affixes::Teeming:0;
            Scenario* scenario=nullptr;
            if(profile){scenario=sScenarioMgr->AddScenario(map.get(),1175);Require(scenario,"Native scenario registration failed");if(profile>=2)scenario->CreateChallenge(player.get());}
            if(profile>=4){scenario->GetChallenge()->_run=true;scenario->GetChallenge()->_complete=profile==5;}
                data.isTeemingSpawn=true;sObjectMgr->NewOrExistCreatureData(data.guid)=data;
                bool loaded=LoadSpawn(data,*map);
                std::cout<<"INSPECT difficulty="<<unsigned(difficulty)<<" profile="<<profile<<" flaggedLoaded="<<loaded<<'\n';
#ifdef TEEMING_BASELINE
                Check(loaded==(profile==0||profile==2),"Original absent-scenario/mode fail-open not reproduced");
#else
                Check(loaded==(difficulty==DIFFICULTY_MYTHIC_KEYSTONE&&(profile==2||profile>=4)),"Native flagged spawn escaped exact Keystone/Teeming admission");
                data.isTeemingSpawn=false;sObjectMgr->NewOrExistCreatureData(data.guid)=data;
                Check(LoadSpawn(data,*map),"Teeming gate rejected an ordinary native spawn");
#endif
            if(scenario){map->m_scenarios.erase(scenario);sScenarioMgr->RemoveScenario(map->GetInstanceId());}
        }
#ifdef TEEMING_BASELINE
        std::cout<<"COUNTERFACTUAL "<<checked<<" native Teeming checks:cache-only flagged real ordinary record loads without scenario and on non-Keystone maps;live Teeming placements remain missing\n";
#else
        // Before scenario publication rejection must be retryable. The key's
        // actual countdown is sufficient; the timed run need not have started.
        {
            MapInstanced parent(1492,300);InstanceMap map(1492,300,9944000,DIFFICULTY_MYTHIC_KEYSTONE,&parent);map.CreateInstanceData(nullptr);
            LifecyclePlayer owner(map,803,Position{2934.49f,715.757f,554.286f,1.55255f});auto key=owner.Key(*sMapChallengeModeStore.LookupEntry(208));owner.m_challengeKeyInfo.Affix=Affixes::Teeming;
            data.isTeemingSpawn=true;sObjectMgr->NewOrExistCreatureData(data.guid)=data;Check(!LoadSpawn(data,map),"Flagged spawn loaded before scenario publication");
            auto scenario=sScenarioMgr->AddScenario(&map,1175);Require(scenario,"Retry scenario registration failed");scenario->CreateChallenge(&owner);
            Check(!scenario->GetChallenge()->_run&&LoadSpawn(data,map),"Pre-scenario denial consumed valid countdown retry");
            map.m_scenarios.erase(scenario);sScenarioMgr->RemoveScenario(map.GetInstanceId());Check(!LoadSpawn(data,map),"Removed challenge retained flagged-spawn admission");
        }
        // Instance-zero used to bypass the Teeming block altogether.
        {
            MapInstanced parent(1492,300);InstanceMap map(1492,300,0,DIFFICULTY_NORMAL,&parent);map.CreateInstanceData(nullptr);
            data.isTeemingSpawn=true;sObjectMgr->NewOrExistCreatureData(data.guid)=data;Check(!LoadSpawn(data,map),"Instance-zero bypassed Teeming denial");
            data.isTeemingSpawn=false;sObjectMgr->NewOrExistCreatureData(data.guid)=data;Check(LoadSpawn(data,map),"Instance-zero ordinary DB path regressed");
        }

        std::cout<<"PASS "<<checked<<" native Teeming admission checks;cache marker controlled,live placements still missing\n";
#endif
        CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
    }catch(std::exception const&error){std::cerr<<error.what()<<'\n';return 1;}
}
