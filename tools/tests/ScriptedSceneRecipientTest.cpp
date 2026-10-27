#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>
using uint32=uint32_t;using SpellEffIndex=uint32;
constexpr uint32 SCRIPT_COMMAND_PLAYSCENE=35,SCRIPT_COMMAND_STOPSCENE=36,SCENE_LAUNCH=1;
constexpr uint32 SPELL_EFFECT_HANDLE_HIT=2,SPELL_EFFECT_HANDLE_HIT_TARGET=1,TARGET_FLAG_UNIT_MASK=1,TARGET_FLAG_GAMEOBJECT_MASK=2,CRITERIA_TYPE_SCRIPT_EVENT=73,CRITERIA_TYPE_SCRIPT_EVENT_2=92;
uint32 errors=0;
#define TC_LOG_ERROR(...) (++errors)
#define TC_LOG_DEBUG(...) ((void)0)
struct Player;struct WorldObject;
struct Object{virtual~Object()=default;virtual Player*ToPlayer(){return nullptr;}bool IsPlayer(){return ToPlayer()!=nullptr;}uint32 GetTypeId(){return IsPlayer()?4:3;}uint32 GetEntry(){return 0;}uint32 GetGUIDLow(){return 1;}};
struct Position{Position const&GetPosition()const{return*this;}};
struct ScriptInfo{uint32 command=36;struct{uint32 sceneID=1109;}PlayScene;std::string GetDebugInfo()const{return "event_scripts46900";}};
struct Map{uint32 calls=0;void Execute(Object*source,Object*target){ScriptInfo script;Run(source,target,script);}void Run(Object*,Object*,ScriptInfo const&);Player*_GetScriptPlayer(Object*,bool,ScriptInfo const*)const;void ScriptsStart(int,uint32 id,Object*s,Object*t){assert(id==46900);++calls;Execute(s,t);}};
struct SpellScene{uint32 PlaybackFlags=11,SceneScriptPackageID=1489;};struct SpellMgr{SpellScene scene;SpellScene const*GetSpellScene(uint32 id){return id==1109||id==1144?&scene:nullptr;}}mgr;auto*sSpellMgr=&mgr;
struct Packet{bool cancel=false;uint32 id=0;Packet const*Write()const{return this;}};
namespace WorldPackets{namespace Scene{struct PlayScene:Packet{uint32 SceneID=0,PlaybackFlags=0,SceneInstanceID=0,SceneScriptPackageID=0;Position Pos;};struct CancelScene:Packet{explicit CancelScene(uint32 i){cancel=true;id=i;}};}}
struct ZoneScript{void ProcessEvent(WorldObject*,uint32){}};using InstanceScript=ZoneScript;
struct WorldObject:Object{Map*map=nullptr;Map*GetMap(){return map;}};
struct Unit:WorldObject{ZoneScript*GetZoneScript(){return nullptr;}InstanceScript*GetInstanceScript(){return nullptr;}};
struct Player:Unit,Position{uint32 sceneInstanceID=0,plays=0,criteria=0;std::vector<uint32>cancelled;std::map<uint32,uint32>m_sceneInstanceID,m_sceneStatus;Player*ToPlayer()override{return this;}void SendSpellScene(uint32,struct SpellInfo const*,bool,Position*);void SceneCompleted(uint32){assert(false);}void SendDirectMessage(Packet const*p){if(p->cancel)cancelled.push_back(p->id);else ++plays;}void UpdateAchievementCriteria(uint32,uint32,uint32){++criteria;}};
#include "SendScene.inc"
#include "Recipient.inc"
void Map::Run(Object*source,Object*target,ScriptInfo const&info){struct{ScriptInfo const*script;}step{&info};switch(info.command){
#include "Commands.inc"
}}
struct Effect{uint32 MiscValue=46900;uint32 GetProvidedTargetMask()const{return 0;}};struct SpellInfo{uint32 Id=193711;Effect effect;Effect const*GetEffect(uint32,uint32)const{return &effect;}};
struct Spell{Unit*m_caster;Unit*unitTarget=nullptr;WorldObject*gameObjTarget=nullptr,*focusObject=nullptr;uint32 effectHandleMode=SPELL_EFFECT_HANDLE_HIT,m_diffMode=0;SpellInfo info;SpellInfo*m_spellInfo=&info;explicit Spell(Player*p):m_caster(p){}void EffectSendEvent(uint32);};int sEventScripts=0;
#include "SendEvent.inc"
int main(){
 Map map;Player source,target;Unit npc;source.map=target.map=npc.map=&map;
 source.SendSpellScene(1109,nullptr,true,&source);source.SendSpellScene(1144,nullptr,true,&source);source.SendSpellScene(1109,nullptr,true,&source);
 Spell event(&source);event.EffectSendEvent(2);
#ifdef BASELINE
 assert(errors==1&&source.cancelled.empty()&&source.m_sceneInstanceID.size()==3);std::cout<<"COUNTERFACTUAL actual targetless193711 SendEvent46900 leaves target null: old STOPSCENE rejects it despite real Player source.\n";
#else
 assert(!errors&&(source.cancelled==std::vector<uint32>{1,3})&&source.m_sceneInstanceID.size()==1&&source.m_sceneInstanceID.at(2)==1144);
 assert(source.criteria==2); // the unchanged native event dispatch, no new helper grants
 event.EffectSendEvent(2);assert(!errors&&source.cancelled.size()==2); // native stop is idempotent
 ScriptInfo play;play.command=35;map.Run(&npc,&target,play);assert(target.plays==1&&source.plays==3);
 ScriptInfo stop;map.Run(&source,&target,stop);assert(target.cancelled==std::vector<uint32>{1}&&source.cancelled.size()==2);
 auto before=errors;map.Run(&source,&npc,stop);assert(errors==before+1&&target.cancelled.size()==1);
 map.Run(&npc,nullptr,stop);map.Run(nullptr,nullptr,stop);assert(errors==before+3);
 std::cout<<"PASS actual Map scene recipient/Player SendSpellScene/native targetless event: source-player stop, normal NPC-source/player-target play, explicit-target precedence, both scene instances/unrelated scene retention, idempotence and retained invalid-target diagnostics. No nearest player, progression, teleport or client visual proof.\n";
#endif
}
