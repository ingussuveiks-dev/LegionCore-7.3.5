#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using int32=int32_t;using uint32=uint32_t;
#define _TRINITY_CORE_CONFIG "fixture"
unsigned errors=0;
#define TC_LOG_ERROR(...) (++errors)
#define TC_LOG_INFO(...) ((void)0)
uint32 getMSTime(){return 0;}uint32 GetMSTimeDiffToNow(uint32){return 0;}
struct Config {int GetIntDefault(char const*,int){return 0;}} config;auto sConfigMgr=&config;
static void Require(bool ok,char const* why){if(!ok)throw std::runtime_error(why);}
#include "PoiTypes.inc"
#include "PoiConstructors.inc"
using QuestPOIVector=std::vector<QuestPOI>;
struct Field {int32 value;int32 GetInt32(){return value;}bool GetBool(){return value!=0;}};
struct Result {std::vector<std::vector<Field>> rows;size_t index=0;Field* Fetch(){return rows.at(index).data();}bool NextRow(){return ++index<rows.size();}};
using QueryResult=std::shared_ptr<Result>;
struct Database {
 QueryResult poi,points;
 QueryResult Query(char const* sql){return std::string(sql).find("FROM quest_poi_points")!=std::string::npos?points:poi;}
}WorldDatabase;
namespace Trinity {namespace Containers {
 template<class T,class K>auto MapGetValuePtr(T& map,K key)->typename T::mapped_type*{auto it=map.find(key);return it==map.end()?nullptr:&it->second;}
}}
struct QuestDataStoreMgr {
 std::map<uint32,QuestPOIVector> _questPOIStore;
 void LoadQuestPOI();QuestPOIVector const* GetQuestPOIVector(int32);
}store;
#include "PoiLoader.inc"
#include "PoiLookup.inc"
namespace WorldPackets {namespace Query {
 struct QuestPOIBlobPoint {int32 X=0,Y=0;};
 struct QuestPOIBlobData:QuestPOI {std::vector<QuestPOIBlobPoint> QuestPOIBlobPointStats;};
 struct QuestPOIData {int32 QuestID=0;std::vector<QuestPOIBlobData> QuestPOIBlobDataStats;};
 struct Response {std::vector<QuestPOIData> QuestPOIDataStats;};
}}
static WorldPackets::Query::Response BuildResponse(int32 QuestID){
 auto poiData=store.GetQuestPOIVector(QuestID);Require(poiData,"Missing native loaded POIs");WorldPackets::Query::Response response;
#include "PoiResponse.inc"
 return response;
}
static QueryResult Read(char const* path,unsigned columns){
 std::ifstream file(path);Require(bool(file),"Fixture input missing");auto result=std::make_shared<Result>();std::string line;
 while(std::getline(file,line)){if(line.empty())continue;std::istringstream row(line);std::vector<Field> fields;int32 value;
  while(row>>value)fields.push_back({value});Require(fields.size()==columns,"Bad numeric fixture column count");result->rows.push_back(fields);}
 return result;
}
int Run(int argc,char**argv){Require(argc==4,"Expected before/after/points");
 auto points=Read(argv[3],4);
 // The real database has quests above44923. Supply a higher row so the
 // unchanged native maximum-quest point-loader boundary matches that context.
 points->rows.insert(points->rows.begin(),{{44924},{0},{0},{0}});
 std::vector<std::vector<WorldPackets::Query::QuestPOIBlobPoint>> beforePoints;
 for(unsigned pass=0;pass<2;++pass){errors=0;WorldDatabase.poi=Read(argv[pass+1],15);WorldDatabase.points=std::make_shared<Result>(*points);
  store.LoadQuestPOI();Require(!errors,"Native loader missing-point error");
  auto response=BuildResponse(44923);Require(response.QuestPOIDataStats.size()==1,"Native response missing quest");
  auto const& blobs=response.QuestPOIDataStats[0].QuestPOIBlobDataStats;
  Require(blobs.size()==(pass?4u:5u),"Unexpected response blob count");unsigned bad=0,valid=0;
  for(auto const& blob:blobs){if(blob.MapID==-117440508){++bad;Require(blob.QuestObjectiveID==-2063596446&&blob.QuestPOIBlobPointStats.size()==10,"Wrong corruption reproduction");continue;}
   Require(blob.MapID==1220&&(blob.WorldMapAreaID==1015||blob.WorldMapAreaID==1017),"Invalid retained map");
   Require((blob.ObjectiveIndex==0&&blob.QuestObjectiveID==287303&&blob.QuestObjectID==115845&&
    blob.QuestPOIBlobPointStats.size()==(blob.WorldMapAreaID==1015?9u:10u))||
    (blob.ObjectiveIndex==1&&blob.QuestObjectiveID==287309&&blob.QuestObjectID==115846&&blob.QuestPOIBlobPointStats.size()==1)||
    (blob.ObjectiveIndex==32&&!blob.QuestObjectiveID&&blob.QuestPOIBlobPointStats.size()==1),"Retained native objective/pin/points changed");++valid;
   if(!pass)beforePoints.push_back(blob.QuestPOIBlobPointStats);else{
    auto const& saved=beforePoints.at(valid-1);auto const& current=blob.QuestPOIBlobPointStats;Require(saved.size()==current.size(),"Point count changed");
    for(size_t i=0;i<saved.size();++i)Require(saved[i].X==current[i].X&&saved[i].Y==current[i].Y,"Polygon point changed");}
  }
  Require(valid==4&&bad==(pass?0u:1u),"Wrong native response repair");
 }
 std::cout<<"PASS: actual native POI loader accepts the corrupt map/objective row and its10 shared points without a DB error; actual response construction exposes it. Migration leaves4 valid blobs and every existing polygon/pin coordinate intact. Query/field/config/request services remain fixtures; no packet transport or client claim.\n";
 return 0;
}
int main(int argc,char**argv){try{return Run(argc,argv);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
