// Actual finite aura430 expiry vs a late native scene-complete message.
#define main RetainedPublicationMain
#include "WarriorPickupPublicationNative.cpp"
#include <cstring>
#undef main
struct ScenePlayer : NativePlayer { using NativePlayer::NativePlayer;using Player::m_sceneInstanceID;using Player::sceneInstanceID; };
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 sSpellMgr->LoadSpellScene();
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1511,{}},{1495,{}}});
 MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1511,{}},{1495,{}}});
 auto session=Session();uint32 cases=0;
 for(bool protection:{false,true})for(uint32 mode=0;mode<5;++mode){
 uint32 mapId=protection?1495:1511,spellId=protection?205340:205443,sceneId=protection?1215:1218;
 MapInstanced parent(mapId,300);InstanceMap map(mapId,300,9986801,DIFFICULTY_N_SCENARIO,&parent);
 {
  ScenePlayer player(session,map,902,{3515.24f,1800.51f,0.538696f,0});
  player.SetByteValue(UNIT_FIELD_BYTES_0,1,mode==3?CLASS_ROGUE:CLASS_WARRIOR);
  auto spell=sSpellMgr->GetSpellInfo(spellId);Require(spell&&spell->GetDuration()==9000,"Matching finite Warrior aura changed");
  Require(player.CastSpell(&player,spellId,true)==SPELL_CAST_OK,"Real pickup aura cast failed");
  for(uint32 t=0;t<100;t+=10)player.Unit::Update(10);
  Require(player.HasAura(spellId)&&player.HasSceneStatus(sceneId,SCENE_LAUNCH),"Native scene430 did not launch");
  auto instance=player.sceneInstanceID;
  if(mode==4){auto pos=player.GetPosition();player.SendSpellScene(sceneId,spell,true,&pos);instance=player.sceneInstanceID;}
  Require(player.m_sceneInstanceID.count(instance),"Native scene instance receipt missing");
  auto application=player.GetAura(spellId)->GetApplicationOfTarget(player.GetGUID());
  Require(application,"Real finite aura application missing");
  // Independently expose the native bitfield layout used by the inlined
  // GetRemoveMode in the linked handler. Restore it before any lifecycle tick.
  auto originalMode=application->GetRemoveMode();
  std::vector<uint8> before(sizeof(AuraApplication));std::memcpy(before.data(),application,before.size());
  application->SetRemoveMode(AURA_REMOVE_BY_EXPIRE);
  auto raw=reinterpret_cast<uint8 const*>(application);uint32 changed=0;
  for(uint32 i=0;i<before.size();++i)if(before[i]!=raw[i]){Require(i==0x10&&raw[i]==4,"Native AuraApplication remove-mode bitfield layout changed");++changed;}
  Require(changed==1&&application->GetRemoveMode()==AURA_REMOVE_BY_EXPIRE,"Native removal mode getter/setter mismatch");
  application->SetRemoveMode(originalMode);
  if(mode==1)player.RemoveAurasDueToSpell(spellId,ObjectGuid::Empty,0,AURA_REMOVE_BY_CANCEL);
  if(mode==2)player.SceneCompleted(instance);
  for(uint32 t=0;t<9100;t+=10)player.Unit::Update(10);
  Require(!player.HasAura(spellId),"Native9000ms aura failed natural expiry");
  auto completed=player.HasSceneStatus(sceneId,SCENE_COMPLETE);
  player.SceneCompleted(instance); // The exact native packet endpoint after expiry.
#ifdef WARSWORDS_SCENE_EXPIRY_BASELINE
  bool expected=mode==2;
#elif defined(WARSWORDS_PROTECTION_EXPIRY_BASELINE)
  bool expected=mode==2||(!protection&&(mode==0||mode==4));
#else
  bool expected=mode==0||mode==2||mode==4;
#endif
  Require(completed==expected&&player.HasSceneStatus(sceneId,SCENE_COMPLETE)==expected&&player.m_sceneInstanceID.empty(),"Actual finite Warrior lifecycle acknowledgement differs");
  std::cout<<"CASE native spell="<<spellId<<" scene="<<sceneId<<" mode="<<mode<<" complete="<<completed<<" late/repeat harmless;no character save\n";++cases;
 }
 }
 std::cout<<"PASS "<<cases<<" real finite Aura/Player scene lifecycle contexts;not rendered playback or full quest traversal\n";
 session->SetPlayer(nullptr);delete session;CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
