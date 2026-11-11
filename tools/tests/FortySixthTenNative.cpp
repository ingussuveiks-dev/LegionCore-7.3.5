// Actual core owner/charmer lookup and victim-instance getter projections.
// Full death payloads are covered separately with controlled endpoints.
#include "ActorQueueLifetimeNative.h"
#include "InstanceScript.h"
#include "FortySixthNativeSelectors.inc"
struct DeathInstanceTag{using Type=InstanceScript* InstanceMap::*;friend Type Access(DeathInstanceTag);};
template struct FixtureAccess<DeathInstanceTag,&InstanceMap::i_data>;
template<class Selector>static void OwnerProjection(Unit*killer,Player*expected){Require(Selector::Resolve(killer)==expected,"actual source selector returns eligible player/owner/charmer or null");}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 // Read the matching native catalog;this does not update character progress.
 // Sentinel's original custom call supplies default miscValue2=0. Keep the
 // ordinary KillRewarder/KilledMonsterCredit amount1 route independent.
 for(auto criterion:sCriteriaStore)if(criterion->Asset==53085&&criterion->Type==CRITERIA_TYPE_KILL_CREATURE){
  std::cout<<"SENTINEL_CRITERION "<<criterion->ID<<" modifier="<<criterion->ModifierTreeId<<'\n';
  for(auto tree:sCriteriaTreeStore)if(tree->CriteriaID==criterion->ID)std::cout<<"SENTINEL_TREE "<<tree->ID<<" amount="<<tree->Amount<<" parent="<<tree->Parent<<'\n';
 }
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});
 MapInstanced parent(1600,300);InstanceMap map(1600,300,9999772,DIFFICULTY_N_SCENARIO,&parent);
 Require(!map.GetInstanceScript(),"controlled map starts without an instance script");map.*Access(DeathInstanceTag{})=new InstanceScript(&map);
 Actor actor(&map,299,11911,{52,803,805,0}),pet(&map,299,11912,{53,803,805,0}),guardian(&map,299,11913,{54,803,805,0}),charmed(&map,299,11914,{55,803,805,0}),unowned(&map,299,11915,{56,803,805,0});
 for(auto*u:{&actor,&pet,&guardian,&charmed,&unowned})Require(map.AddToMap(static_cast<Creature*>(u)),"insert native actor");
 auto session=Session();{
 NativePlayer player(session,map,1100,{50,803,805,0});pet.SetGuidValue(UNIT_FIELD_CREATED_BY,player.GetGUID());guardian.SetGuidValue(UNIT_FIELD_CREATED_BY,pet.GetGUID());charmed.SetCharmerGUID(player.GetGUID());
 for(Unit*killer:{static_cast<Unit*>(&player),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian),static_cast<Unit*>(&charmed),static_cast<Unit*>(&unowned),static_cast<Unit*>(nullptr)}){
  Player*expected=killer&&killer!=&unowned?&player:nullptr;
  OwnerProjection<MoltenBehemothDeathCredit>(killer,expected);
OwnerProjection<FlamewakerSentinelDeathCredit>(killer,expected);
OwnerProjection<FlamewakerShamanDeathCredit>(killer,expected);
OwnerProjection<SilverHandDeathCredit>(killer,expected);
OwnerProjection<VashjelanSirenDeathCredit>(killer,expected);
OwnerProjection<AshranChampionDeathRecruit>(killer,expected);
OwnerProjection<AncientRegailDeathBind>(killer,expected);
OwnerProjection<AncientAsaniDeathBind>(killer,expected);
OwnerProjection<ProtectorKaolanDeathBind>(killer,expected);
  Require(MagmusVictimInstance::Resolve(&actor,killer)==map.GetInstanceScript(),"victim's actual instance is independent of nullable killer");
 }
 // The witness sets only the real charmer field,without installing a charm AI.
 // Restore it before teardown,which correctly rejects dangling charm links.
 charmed.SetCharmerGUID(ObjectGuid::Empty);
 }
 std::vector<std::pair<uint32,std::string>> expectedBindings={{9938,"boss_magmus"},{17910,"boss_silver_hand_bosses"},{17911,"boss_silver_hand_bosses"},{17912,"boss_silver_hand_bosses"},{17913,"boss_silver_hand_bosses"},{17914,"boss_silver_hand_bosses"},{38360,"npc_vashjelan_siren"},{52552,"npc_molten_behemoth"},{53085,"npc_flamewaker_sentinel"},{53093,"npc_flamewaker_shaman"},{60583,"boss_protector_kaolan"},{60585,"boss_ancient_regail"},{60586,"boss_ancient_asani"},{81725,"npc_ashran_faction_champions"},{81726,"npc_ashran_faction_champions"}};auto receipt=WorldDatabase.Query("SELECT entry,ScriptName FROM creature_template WHERE ScriptName IN('npc_molten_behemoth','npc_flamewaker_sentinel','npc_flamewaker_shaman','boss_silver_hand_bosses','npc_vashjelan_siren','npc_ashran_faction_champions','boss_ancient_regail','boss_ancient_asani','boss_protector_kaolan','boss_magmus') ORDER BY entry,ScriptName");Require(receipt&&receipt->GetRowCount()==expectedBindings.size(),"Exact native death binding inventory");uint32 row=0;do{auto f=receipt->Fetch();Require(f[0].GetUInt32()==expectedBindings[row].first&&f[1].GetString()==expectedBindings[row].second,"Death binding changed");++row;}while(receipt->NextRow());
 CloseAltheaNative();std::cout<<"PASS native ten selected cases:real player/pet/two-level guardian/charmer/null/unowned resolution and victim-instance identity;exact live bindings,no progression/character writes. Selector projections only,not full death payload/client traversal\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
