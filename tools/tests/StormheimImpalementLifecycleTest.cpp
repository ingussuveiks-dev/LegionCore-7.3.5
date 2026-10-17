#include "ImpalementLifecycleFixture.inc"
namespace ObjectAccessor {Player* GetPlayer(GameObject&,ObjectGuid){assert(false);return nullptr;}}
#include "ImpalementDespawnAtAction.inc"
#include "ImpalementSpawned.inc"
void GameObject::Tick()
{
    switch(m_lootState){
        case GO_READY:{
            #include "ImpalementRespawn.inc"
            break;
        }
        #include "ImpalementDeactivate.inc"
    }
}
int main()
{
    store.quests[38410].id=38410;store.quests[38615].id=38615;
    for(auto team:{ALLIANCE,HORDE}){
        GameTime::now=1000;lifecycleWorld={};Run first(team),other(team);auto& object=first.object;
        assert(object.info.IsDespawnAtAction()&&object.isSpawned());first.Open();assert(first.p.counts[first.Quest()]==1&&object.state==GO_JUST_DEACTIVATED);
        object.Tick();assert(object.state==GO_READY&&object.m_respawnTime==1180&&!object.isSpawned());
        assert(object.flags==262144&&object.animations==1&&object.visibility==1&&object.loot.clears==1&&object.saves==1);
        // IN_USE has already been reset by native Update; native isSpawned,
        // rather than manually reset fixture flags, must prevent another hit.
        Spell second(&other.p,&object);second.Complete();first.Open();assert(!other.p.counts[other.Quest()]&&first.p.counts[first.Quest()]==1);
        GameTime::now=1179;object.Tick();assert(!object.isSpawned()&&!object.mapState.adds);second.Complete();assert(!other.p.counts[other.Quest()]);
        GameTime::now=1180;assert(!object.isSpawned());object.Tick();assert(object.isSpawned()&&object.m_respawnTime==0&&object.mapState.adds==1);
        second.Complete();assert(other.p.counts[other.Quest()]==1&&object.state==GO_JUST_DEACTIVATED);second.Complete();assert(other.p.counts[other.Quest()]==1);
        object.Tick();assert(!object.isSpawned()&&object.m_respawnTime==1360&&object.animations==2&&object.saves==2);
        // Saved timestamp reconstruction executes the same native spawn/timer
        // predicate; storage, map insertion and packet delivery remain fixtures.
        Run reloaded(team);reloaded.object.m_respawnTime=object.m_respawnTime;reloaded.object.state=GO_READY;
        assert(!reloaded.object.isSpawned());GameTime::now=1360;reloaded.object.Tick();assert(reloaded.object.isSpawned());reloaded.Open();assert(reloaded.p.counts[reloaded.Quest()]==1);
    }
    for(auto players:{0u,10u,20u}){
        GameTime::now=2000;lifecycleWorld={true,false,players};Run r;r.Open();r.object.Tick();
        auto delay=players==0?180:players==10?18:12;assert(r.object.m_respawnTime==2000+delay&&!r.object.saves&&!r.object.isSpawned());
    }
    {GameTime::now=3000;lifecycleWorld={true,true,20};Run r;r.object.mapState.instance=true;r.Open();r.object.Tick();assert(r.object.m_respawnTime==3180);}
    std::cout<<"PASS: actual GO_JUST_DEACTIVATED, IsDespawnAtAction, isSpawned and GO_READY timer branches preserve180s shared launcher respawn; post-reset duplicate/second-player hits blocked until native timer update, saved-timestamp reconstruction and adaptive18s/12s floor/instance/save settings pass. World/storage/clock/RNG/packets remain fixtures; client animation pending.\n";
}
