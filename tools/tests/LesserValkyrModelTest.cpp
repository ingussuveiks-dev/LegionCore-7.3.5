#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
using uint8=uint8_t; using uint32=uint32_t; using int32=int32_t;
unsigned errors=0, randomModel=0;
uint32 urand(uint32 min,uint32 max){return min==0&&max==1 ? randomModel : min;}
#define TC_LOG_ERROR(...) (++errors)
constexpr uint32 CREATURE_FLAG_EXTRA_TRIGGER=128, UNIT_FIELD_FLAGS_2=0, UNIT_FLAG2_MIRROR_IMAGE=0;
int32 HacksMorphesOne[31]={},HacksMorphesTwo[31]={};
struct CreatureTemplate {
    uint32 Entry=45239,flags_extra=0; int32 Modelid[4]={25517,33997,0,0};
    int32 GetRandomValidModelId()const; int32 GetFirstValidModelId()const;
    uint32 GetFirstInvisibleModel()const{return 11686;}
};
struct CreatureData {uint32 displayid=0;};
struct CreatureModelInfo {uint8 gender=1;uint32 displayId_other_gender=0;};
using CreatureOutfitContainer=std::map<int,int>;
struct ObjectMgr {
    std::map<uint32,CreatureModelInfo> models;
    CreatureModelInfo const* GetCreatureModelInfo(uint32 id){auto it=models.find(id);return it==models.end()?nullptr:&it->second;}
    CreatureOutfitContainer const& GetCreatureOutfitMap()const{static CreatureOutfitContainer empty;return empty;}
    static int32 ChooseDisplayId(uint32,CreatureTemplate const*,CreatureData const*);
    uint32 GetCreatureDisplay(int32)const;
    CreatureModelInfo const* GetCreatureModelRandomGender(uint32*);
};
ObjectMgr manager;ObjectMgr* sObjectMgr=&manager;
#include "GetRandomValidModelId.inc"
#include "GetFirstValidModelId.inc"
#include "ChooseDisplayId.inc"
#include "GetCreatureDisplay.inc"
#include "GetCreatureModelRandomGender.inc"
struct Creature {
    uint32 outfit=0,display=0,native=0;uint8 gender=0;
    void SetOutfit(uint32 value){outfit=value;}uint32 GetOutfit()const{return outfit;}
    bool IsMirrorImage()const{return false;}void RemoveFlag(uint32,uint32){}
    void SetDisplayId(uint32 value){display=value;}void SetNativeDisplayId(uint32 value){native=value;}void SetGender(uint8 value){gender=value;}
    bool InitModel(CreatureTemplate const* cinfo,CreatureData const* data){
        uint32 entry=cinfo->Entry;
        (void)entry; // Used by production diagnostics; test logger only counts errors.
#include "InitEntryModel.inc"
        return true;
    }
};
int main(int argc,char** argv){
    assert(argc==3);
    std::ifstream modelFile(argv[1]);assert(modelFile);
    uint32 id,other;float radius,reach;unsigned hostile;
    while(modelFile>>id>>radius>>reach>>other>>hostile)manager.models.emplace(id,CreatureModelInfo{1,other});
    assert(manager.models.count(25517)&&manager.models.count(33997)&&!manager.models.count(26101));
    CreatureTemplate definition;CreatureData invalid{26101};Creature old;
    assert(!old.InitModel(&definition,&invalid)&&errors==1); // Same second failure branch as DBErrors.log.
    std::ifstream input(argv[2]);assert(input);unsigned before,after,count=0,repaired=0;
    while(input>>before>>after){
        CreatureData data{after};Creature creature;
        for(randomModel=0;randomModel<2;++randomModel){
            assert(creature.InitModel(&definition,&data));
            assert(creature.display==(after?after:definition.Modelid[randomModel]));
            assert(creature.native==creature.display&&creature.gender==1);
        }
        if(before==26101){assert(after==0);++repaired;}else assert(before==after);
        ++count;
    }
    assert(count>=15&&repaired>0&&errors==1);
    CreatureData valid{25517};Creature preserved;assert(preserved.InitModel(&definition,&valid)&&preserved.display==25517);
    std::cout<<"PASS: production spawn-model override, template selection and InitEntry model branch reproduce 26101 failure; every repaired spawn and both template variants initialize.\n";
}
