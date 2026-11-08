// Pinned pre-repair Prepare capsule, not a replacement Player COMDAT or a
// historical whole binary. Other native components use current game libraries.
#include "AcquisitionScenarioRoute.h"
#include "BrokenShoreClassRoute.h"
struct TuureLoginIdTag{using Type=uint32 Player::*;friend Type Access(TuureLoginIdTag);};
template struct FixtureAccess<TuureLoginIdTag,&Player::m_scenarioLoginId>;
struct TuureLoginInstanceTag{using Type=uint32 Player::*;friend Type Access(TuureLoginInstanceTag);};
template struct FixtureAccess<TuureLoginInstanceTag,&Player::m_scenarioLoginInstance>;
namespace PinnedTuureLogin {
#include "TuurePinnedNamespace.inc"
static void Prepare(Player& player,uint32 mapId,uint32 instanceId){
 auto& m_scenarioResume=player.*Access(TuureEntryTag{});auto& m_scenarioLoginId=player.*Access(TuureLoginIdTag{});auto& m_scenarioLoginInstance=player.*Access(TuureLoginInstanceTag{});
 auto getClass=[&]{return player.getClass();};auto SetScenarioId=[&](uint16 id){player.SetScenarioId(id);};auto Relocate=[&](float x,float y,float z,float o){player.Relocate(x,y,z,o);};auto GetGUIDLow=[&]{return player.GetGUIDLow();};
#include "TuurePinnedPrepare.inc"
}
}
