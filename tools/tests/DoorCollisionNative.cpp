// Real native GO pose/loot states and the matching4413 collision mesh.
#define ROCKAWAY_NATIVE_SUPPORT_ONLY
#include "RockawayOffensiveNative.cpp"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "GameObjectModel.h"
#include "MapInstanced.h"
#include <filesystem>
#include <cstdlib>
struct CollisionDoor:GameObject{~CollisionDoor(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<GameObject*>(this),false);ResetMap();}};
static bool MeshHit(GameObject& object){
 auto const& box=object.m_model->getBounds();auto lo=box.low(),hi=box.high();
 for(float z:{0.25f,0.5f,0.75f})for(float cross:{0.25f,0.5f,0.75f})for(bool x:{false,true}){
  G3D::Vector3 from,to,result;float height=lo.z+(hi.z-lo.z)*z;
  if(x){float y=lo.y+(hi.y-lo.y)*cross;from={lo.x-2,y,height};to={hi.x+2,y,height};}
  else{float a=lo.x+(hi.x-lo.x)*cross;from={a,lo.y-2,height};to={a,hi.y+2,height};}
  if(object.m_model->getObjectHitPos(object.GetPhases(),false,from,to,result,0))return true;
 }return false;
}
int main(int argc,char**argv){try{
 SetUnhandledExceptionFilter(CrashTrace);Require(argc==2,"Expected dbc directory");auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config,"Private config missing");
 auto dir=std::filesystem::path(argv[1]).lexically_normal();if(dir.filename().empty())dir=dir.parent_path();std::string runtime=dir.parent_path().parent_path().string()+'/';char* args[]={argv[0],config,runtime.data()};InitializeNative(3,args);
 sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();LoadGameObjectModelList(sWorld->GetDataPath());HashMapHolder<GameObject>::SetSize(20000);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1620,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1620,{}}});
 auto input=const_cast<GameObjectTemplate*>(sObjectMgr->GetGameObjectTemplate(251053));Require(input&&input->type==GAMEOBJECT_TYPE_DOOR&&input->displayId==4413,"Matching native door input differs");auto original=*input;input->ScriptId=0; // Isolate the shared state machine, not quest AI.
 uint32 cases=0;
 for(uint32 type:{uint32(GAMEOBJECT_TYPE_DOOR),uint32(GAMEOBJECT_TYPE_BUTTON)}){
  input->type=type;MapInstanced parent(1620,300);InstanceMap map(1620,300,9997000+type,DIFFICULTY_N_SCENARIO,&parent);
  CollisionDoor door;Require(door.Create(2301+type,251053,&map,1,{-8634.95f,419.141f,103.702f,2.23265f},G3D::Quat(0,0,0.89849f,0.4390f),255,GO_STATE_READY),"Native model door create failed");
  Require(map.AddToMap(static_cast<GameObject*>(&door)),"Native model door admission failed");door.Update(5000);Require(door.m_model&&door.m_model->isCollisionEnabled()&&MeshHit(door),"Real closed4413 mesh is not hit");
  if(type==GAMEOBJECT_TYPE_DOOR){
   door.UseDoorOrButton();Require(door.GetGoState()==GO_STATE_ACTIVE&&door.getLootState()==GO_ACTIVATED,"Native activation failed");
#ifdef DOOR_COLLISION_BASELINE
   Require(door.m_model->isCollisionEnabled()&&MeshHit(door),"Original open-pose collision counterfactual differs");std::cout<<"COUNTERFACTUAL native open4413 still blocks actual mesh rays after SetLootState\n";
#else
   Require(!door.m_model->isCollisionEnabled()&&!MeshHit(door),"Open native pose still blocks4413 mesh rays");
#endif
   door.ResetDoorOrButton();Require(door.GetGoState()==GO_STATE_READY,"Native reset pose differs");
#ifdef DOOR_COLLISION_BASELINE
   Require(!door.m_model->isCollisionEnabled()&&!MeshHit(door),"Original reset collision counterfactual differs");std::cout<<"COUNTERFACTUAL native reset4413 is closed but its mesh no longer blocks\n";
#else
   Require(door.m_model->isCollisionEnabled()&&MeshHit(door),"Native reset did not restore closed mesh");
#endif
  }
  for(GOState pose:{GO_STATE_READY,GO_STATE_ACTIVE,GO_STATE_ACTIVE_ALTERNATIVE})for(LootState loot:{GO_NOT_READY,GO_READY,GO_ACTIVATED,GO_JUST_DEACTIVATED}){
   door.SetGoState(pose);door.SetLootState(loot);bool legacy=(pose!=GO_STATE_READY&&(loot==GO_ACTIVATED||loot==GO_JUST_DEACTIVATED))||loot==GO_READY;
   bool expected=legacy;
#ifndef DOOR_COLLISION_BASELINE
   if(type==GAMEOBJECT_TYPE_DOOR)expected=pose==GO_STATE_READY;
#endif
   Require(door.m_model->isCollisionEnabled()==expected,"Native pose/loot collision matrix differs");++cases;
  }
 }
 *input=original;std::cout<<"PASS "<<cases<<" actual native DOOR/BUTTON pose/loot collision contexts, real4413 mesh rays and native reset; fixture isolates ScriptId/type only, no SQL/client traversal claim\n";std::cout.flush();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
