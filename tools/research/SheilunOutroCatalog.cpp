#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <set>
#include <string>
#include <vector>

int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto ro=HotfixDatabase.Query("SELECT @@session.transaction_read_only");if(!ro||!ro->Fetch()[0].GetBool())return 4;
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2",BroadcastTextLoadInfo::Instance());
    DB2Storage<SoundKitNameEntry> soundNames("SoundKitName.db2",SoundKitNameLoadInfo::Instance());
    DB2Storage<SoundKitEntryEntry> sounds("SoundKitEntry.db2",SoundKitEntryLoadInfo::Instance());
    DB2Storage<SummonPropertiesEntry> properties("SummonProperties.db2",SummonPropertiesLoadInfo::Instance());
    DB2Storage<TaxiNodesEntry> nodes("TaxiNodes.db2",TaxiNodesLoadInfo::Instance());
    DB2Storage<TaxiPathEntry> paths("TaxiPath.db2",TaxiPathLoadInfo::Instance());
    DB2Storage<TaxiPathNodeEntry> points("TaxiPathNode.db2",TaxiPathNodeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
    DB2Storage<MapEntry> maps("Map.db2",MapLoadInfo::Instance());
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<WorldMapAreaEntry> areas("WorldMapArea.db2",WorldMapAreaLoadInfo::Instance());
    DB2Storage<WorldSafeLocsEntry> safe("WorldSafeLocs.db2",WorldSafeLocsLoadInfo::Instance());
    if(!texts.Load(argv[1],0)||!soundNames.Load(argv[1],0)||!sounds.Load(argv[1],0)||!properties.Load(argv[1],0)||!nodes.Load(argv[1],0)||!paths.Load(argv[1],0)||!points.Load(argv[1],0)||!criteria.Load(argv[1],0)||!maps.Load(argv[1],0)||!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!areas.Load(argv[1],0)||!safe.Load(argv[1],0))return 5;
    texts.LoadFromDB();soundNames.LoadFromDB();sounds.LoadFromDB();properties.LoadFromDB();nodes.LoadFromDB();paths.LoadFromDB();points.LoadFromDB();criteria.LoadFromDB();
    maps.LoadFromDB();
    spells.LoadFromDB();effects.LoadFromDB();
    areas.LoadFromDB();
    safe.LoadFromDB();
    std::cout.precision(9);
    for(auto loc:safe)if(loc->ID==1324||loc->MapID==1514||loc->MapID==1541)
        std::cout<<"SAFE_LOC\t"<<loc->ID<<'\t'<<loc->MapID<<'\t'<<loc->Loc.X<<'\t'<<loc->Loc.Y<<'\t'<<loc->Loc.Z<<'\t'<<loc->Loc.O<<'\t'<<loc->Name->Str[0]<<'\n';
    // Effect252 uses the spell's actual destination/World target-position row.
    // Its common MiscValue1324 is not a WorldSafeLocs foreign key.
    std::set<uint32> transit{194011,194151,199465,202831,210115,215788,217820,202828,202693,202742};
    for(auto spell:spells)
    {
        std::string name=spell->Name->Str[0];
        std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(std::tolower(c));});
        if(spell->ID>=190000&&spell->ID<=240000&&name.find("kite")!=std::string::npos)transit.insert(spell->ID);
    }
    for(auto effect:effects)if(effect->Effect==28&&(effect->EffectMiscValue[0]==102648||effect->EffectMiscValue[0]==102590))transit.insert(effect->SpellID);
    for(auto id:transit)
    {
        if(auto spell=spells.LookupEntry(id))std::cout<<"TRANSIT_SPELL\t"<<id<<'\t'<<spell->Name->Str[0]<<'\n';
        for(auto effect:effects)if(effect->SpellID==id&&!effect->DifficultyID)
            std::cout<<"TRANSIT_EFFECT\t"<<id<<'\t'<<effect->EffectIndex<<'\t'<<effect->Effect<<'\t'<<effect->EffectMiscValue[0]<<'\t'<<effect->EffectMiscValue[1]<<'\t'<<effect->EffectTriggerSpell<<'\t'<<effect->ImplicitTarget[0]<<'\t'<<effect->ImplicitTarget[1]<<'\n';
    }
    std::vector<std::string> const terms={"great service for pandaria","staff of our last emperor","ten thousand years ago, emperor shaohao","will not hide away","emperor's staff against","white tiger bless","back to turtle","like the emperor now","get back to the wandering isle","wookin' grook"};
    std::set<uint32> kits,localNodes,scenarioPaths;
    uint32 textCount=0,selected=0,returnCriteria=0;
    for(auto text:texts)
    {
        ++textCount;std::string value=std::string(text->Text->Str[0])+"\n"+text->Text1->Str[0];
        std::transform(value.begin(),value.end(),value.begin(),[](unsigned char c){return char(std::tolower(c));});
        if(!std::any_of(terms.begin(),terms.end(),[&](std::string const&term){return value.find(term)!=std::string::npos;}))continue;
        ++selected;std::cout<<"TEXT\t"<<text->ID<<'\t'<<unsigned(text->LanguageID)<<'\t'<<text->Text->Str[0]<<'\t'<<text->Text1->Str[0]<<'\n';
        for(auto id:text->SoundEntriesID)if(id>0)kits.insert(id);
        std::cout<<"VOICE\t"<<text->ID<<'\t'<<text->SoundEntriesID[0]<<'\t'<<text->SoundEntriesID[1]<<'\n';
    }
    for(auto id:kits)
    {
        auto name=soundNames.LookupEntry(id);std::cout<<"SOUND\t"<<id<<'\t'<<(name?name->Name->Str[0]:"")<<'\n';
        for(auto sound:sounds)if(sound->SoundKitID==id)std::cout<<"FILE\t"<<id<<'\t'<<sound->FileDataID<<'\t'<<unsigned(sound->Frequency)<<'\n';
    }
    auto prop=properties.LookupEntry(3035);if(!prop)return 6;
    std::cout<<"SUMMON_PROPERTIES\t3035\t"<<prop->Flags<<'\t'<<prop->Control<<'\t'<<prop->Faction<<'\t'<<prop->Title<<'\t'<<prop->Slot<<'\n';
    std::set<int32> candidateMaps{1514,1541};
    for(uint32 id:{1514u,1541u})
    {
        auto map=maps.LookupEntry(id);if(!map)return 7;
        std::cout<<"MAP\t"<<id<<'\t'<<map->ParentMapID<<'\t'<<map->CosmeticParentMapID<<'\t'<<map->Directory->Str[0]<<'\n';
        if(map->ParentMapID>=0)candidateMaps.insert(map->ParentMapID);
        if(map->CosmeticParentMapID>=0)candidateMaps.insert(map->CosmeticParentMapID);
    }
    for(auto area:areas)if(area->MapID==1514||area->MapID==1541)
        std::cout<<"MAP_AREA\t"<<area->ID<<'\t'<<area->MapID<<'\t'<<area->LocLeft<<'\t'<<area->LocRight<<'\t'<<area->LocTop<<'\t'<<area->LocBottom<<'\t'<<area->AreaName->Str[0]<<'\n';
    for(auto node:nodes)if(candidateMaps.count(node->ContinentID))
    {
        localNodes.insert(node->ID);std::cout<<"TAXI_NODE\t"<<node->ID<<'\t'<<node->ContinentID<<'\t'<<node->Pos.X<<'\t'<<node->Pos.Y<<'\t'<<node->Pos.Z<<'\t'<<node->Name->Str[0]<<'\n';
    }
    for(auto point:points)if(candidateMaps.count(point->ContinentID))scenarioPaths.insert(point->PathID);
    for(auto path:paths)if(scenarioPaths.count(path->ID)||localNodes.count(path->FromTaxiNode)||localNodes.count(path->ToTaxiNode))
    {
        std::cout<<"TAXI_PATH\t"<<path->ID<<'\t'<<path->FromTaxiNode<<'\t'<<path->ToTaxiNode<<'\t'<<path->Cost<<'\n';
        for(auto point:points)if(point->PathID==path->ID)std::cout<<"TAXI_POINT\t"<<path->ID<<'\t'<<unsigned(point->NodeIndex)<<'\t'<<point->ContinentID<<'\t'<<point->Loc.X<<'\t'<<point->Loc.Y<<'\t'<<point->Loc.Z<<'\t'<<unsigned(point->Flags)<<'\t'<<point->ArrivalEventID<<'\t'<<point->DepartureEventID<<'\t'<<point->Delay<<'\n';
    }
    for(auto criterion:criteria)if(criterion->Type==92&&criterion->Asset==48658){++returnCriteria;std::cout<<"RETURN_CRITERION\t"<<criterion->ID<<'\t'<<criterion->Asset<<'\n';}
    uint32 taxiSpellCount=0;
    for(auto effect:effects)if(effect->Effect==123&&scenarioPaths.count(effect->EffectMiscValue[0]))
    {
        ++taxiSpellCount;auto spell=spells.LookupEntry(effect->SpellID);
        std::cout<<"TAXI_SPELL\t"<<effect->SpellID<<'\t'<<effect->EffectMiscValue[0]<<'\t'<<unsigned(effect->EffectIndex)<<'\t'<<effect->DifficultyID<<'\t'<<(spell?spell->Name->Str[0]:"")<<'\n';
    }
    std::cout<<"TAXI_SPELL_COUNT\t"<<taxiSpellCount<<'\n';
    std::cout<<"EOF\tsheilun-outro\t"<<textCount<<'\t'<<selected<<'\t'<<localNodes.size()<<'\t'<<scenarioPaths.size()<<'\t'<<returnCriteria<<'\n';
    HotfixDatabase.Close();MySQL::Library_End();
}
