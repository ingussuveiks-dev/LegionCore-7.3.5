// Pinned pre-repair Prepare capsule, not a replacement Player COMDAT or a
// historical whole binary. Other native components use current game libraries.
#include "AcquisitionScenarioRoute.h"
#include "BrokenShoreClassRoute.h"
struct SharasdalLoginIdTag{using Type=uint32 Player::*;friend Type Access(SharasdalLoginIdTag);};
template struct FixtureAccess<SharasdalLoginIdTag,&Player::m_scenarioLoginId>;
struct SharasdalLoginInstanceTag{using Type=uint32 Player::*;friend Type Access(SharasdalLoginInstanceTag);};
template struct FixtureAccess<SharasdalLoginInstanceTag,&Player::m_scenarioLoginInstance>;
namespace PinnedSharasdalLogin {
#include "SharasdalPinnedNamespace.inc"
static void Prepare(Player& player,uint32 mapId,uint32 instanceId){
 auto& m_scenarioResume=player.*Access(SharasdalEntryTag{});auto& m_scenarioLoginId=player.*Access(SharasdalLoginIdTag{});auto& m_scenarioLoginInstance=player.*Access(SharasdalLoginInstanceTag{});
 auto getClass=[&]{return player.getClass();};auto SetScenarioId=[&](uint16 id){player.SetScenarioId(id);};auto Relocate=[&](float x,float y,float z,float o){player.Relocate(x,y,z,o);};auto GetGUIDLow=[&]{return player.GetGUIDLow();};
#include "SharasdalPinnedPrepare.inc"
}
}
