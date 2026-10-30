#define NEXUS_EXIT_TEST
#define main UnusedNexusMageTest
#include "NexusAzuregosReceiverTest.cpp"
#undef main
#include "ExitAddonSelector.inc"
#include "ExitAddonLoader.inc"
bool operator==(Receipt const&a,Receipt const&b){return a.time==b.time&&a.action==b.action&&a.payload==b.payload;}
static void LoadAddons(char const*path){std::ifstream f(path);assert(f);std::string line;while(std::getline(f,line)){std::stringstream s(line);uint32 kind,id;CreatureAddon a;s>>kind>>id>>a.path_id>>a.mount>>a.bytes1>>a.bytes2>>a.emote;uint32 aura;while(s>>aura)a.auras.push_back(aura);(kind?exitObjectMgr.spawn:exitObjectMgr.templates)[id]=a;}}
int main(int argc,char**argv){
 assert(argc==5);LoadAddons(argv[1]);auto old=exitObjectMgr;exitObjectMgr={};LoadAddons(argv[2]);auto fixed=exitObjectMgr;auto events=Rows(argv[3]);auto spawns=Read(argv[4]);uint32 cases=0;
 // Effective addon differential for every shared captured actor. Only the
 // native per-spawn override replaces its inherited wrong aura.
 for(auto const&r:spawns){assert(r.size()==2);Unit u;u.m_DBTableGuid=r[0];u.entry=r[1];exitObjectMgr=old;u.LoadCreaturesAddon();auto before=u.auras;u.auras.clear();exitObjectMgr=fixed;u.LoadCreaturesAddon();if(r[0]==369686){assert(before==std::vector<uint32>{202925});assert(u.auras==std::vector<uint32>{204345});assert(u.GetCreatureAddon()==&exitObjectMgr.spawn.at(369686));}else assert(u.auras==before);auto repeated=u.auras;u.LoadCreaturesAddon(true);assert(u.auras==repeated);++cases;}
 // Actual null-Unit SetData and LINK removal/cast, through native action45
 // after the real Soth death root. GUID/world/phase and door services are
 // controlled; unchanged credit storage and rendered visuals are separate.
 for(uint32 killer=0;killer<4;++killer)for(bool rejected:{false,true}){
  grid.clear();receipts.clear();scriptErrors=0;failCast=rejected;nativeConditions={};hooks={};exitObjectMgr=fixed;scriptMgr.rows=events;
  Unit exit,boss,player,pet,guardian;exit.entry=103151;exit.m_DBTableGuid=369686;exit.LoadCreaturesAddon();SmartScript receiver,sender;receiver.me=&exit;exit.ai.script=&receiver;receiver.mEvents=scriptMgr.GetScript(103151,0);boss.entry=103226;boss.ai.script=&sender;sender.me=&boss;sender.mEvents=scriptMgr.GetScript(103226,0);grid={&exit};player.player=true;pet.owner=guardian.owner=&player;
  assert(exit.HasAura(204345));Unit*k=killer==0?&player:killer==1?&pet:killer==2?&guardian:nullptr;sender.ProcessEventsFor(SMART_EVENT_DEATH,k);
  assert(!exit.HasAura(204345)&&exit.auraRemovals==1&&exit.HasAura(202014)==!rejected);
  assert((receipts==std::vector<Receipt>{{0,205,49052},{0,11,202014},{0,9,5},{0,9,6}}));
  // The pre-existing missing LINK2 remains an asserted unresolved diagnostic.
  uint32 expectedErrors=rejected?0u:1u; // Native failed cast returns before following LINK2.
  assert(scriptErrors==expectedErrors);auto before=receipts.size();exit.ai.SetData(1,1);assert(receipts.size()==before&&exit.auraRemovals==1&&scriptErrors==expectedErrors);++cases;
 }
 for(uint32 mode=0;mode<4;++mode){grid.clear();receipts.clear();scriptErrors=0;failCast=false;nativeConditions={};hooks={};exitObjectMgr=fixed;scriptMgr.rows=events;Unit u;u.entry=103151;u.m_DBTableGuid=369686;u.LoadCreaturesAddon();SmartScript receiver;receiver.me=&u;u.ai.script=&receiver;receiver.mEvents=scriptMgr.GetScript(103151,0);if(mode==0)receiver.me->expired=true;if(mode==1)u.ai.SetData(2,1);if(mode==2)u.ai.SetData(1,2);if(mode==3){u.ai.SetData(0,0);}else if(mode==0)u.ai.SetData(1,1);assert(u.HasAura(204345)&&!u.auraRemovals&&receipts.empty()&&!scriptErrors);++cases;}
 for(uint32 mode=0;mode<4;++mode){grid.clear();receipts.clear();scriptErrors=0;failCast=false;nativeConditions={};hooks={};exitObjectMgr=fixed;scriptMgr.rows=events;Unit u,boss;u.entry=103151;u.m_DBTableGuid=369686;u.LoadCreaturesAddon();SmartScript receiver,sender;receiver.me=&u;u.ai.script=&receiver;receiver.mEvents=scriptMgr.GetScript(103151,0);sender.me=&boss;sender.mEvents=scriptMgr.GetScript(103226,0);if(mode!=0)grid.push_back(&u);if(mode==1)u.map=&otherWorld;if(mode==2)u.phase=2;if(mode==3)u.inWorld=false;sender.ProcessEventsFor(SMART_EVENT_DEATH);assert(u.HasAura(204345)&&!u.auraRemovals&&!scriptErrors);assert(receipts.size()==3);++cases;}
 std::cout<<"PASS "<<cases<<" native addon selection/loading/shared-actor/reload and Soth player/pet/guardian/environment death/SetData/removal/rejected-cast/once cases. Existing missingLINK2 retained explicitly; GUID/door/credit/aura services controlled; actual render/client remains pending.\n";
}
