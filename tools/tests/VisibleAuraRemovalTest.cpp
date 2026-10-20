#include <algorithm>
#include <cstdint>
#include <iostream>
#include <set>
#include <stdexcept>
#include <boost/container/flat_set.hpp>
struct AuraApplication
{
    uint8_t slot;static unsigned reads;
    uint8_t GetSlot() const{++reads;return slot;}
};
unsigned AuraApplication::reads=0;
struct Unit
{
    struct VisibleAuraSlotCompare{bool operator()(AuraApplication*,AuraApplication*) const;};
    std::set<AuraApplication*,VisibleAuraSlotCompare> m_visibleAuras;
    boost::container::flat_set<AuraApplication*,VisibleAuraSlotCompare> m_visibleAurasToUpdate;
    unsigned groupUpdates=0;
    void UpdateAuraForGroup(){++groupUpdates;}
    void RemoveVisibleAura(AuraApplication*);
};
#include "VisibleAuraComparator.inc"
#include "VisibleAuraRemoval.inc"
static void Require(bool ok,char const* msg){if(!ok)throw std::runtime_error(msg);}
int main(){try{
    AuraApplication first{0},replacement{0},other{1};
    Unit normal;normal.m_visibleAuras={&first,&other};normal.m_visibleAurasToUpdate.insert(&first);normal.m_visibleAurasToUpdate.insert(&other);
    normal.RemoveVisibleAura(&first);Require(normal.m_visibleAuras.size()==1&&*normal.m_visibleAuras.begin()==&other&&normal.m_visibleAurasToUpdate.size()==1&&*normal.m_visibleAurasToUpdate.begin()==&other&&normal.groupUpdates==1,"Normal exact application removal lost an unrelated aura");
    Unit reused;reused.m_visibleAuras={&replacement,&other};reused.m_visibleAurasToUpdate.insert(&replacement);reused.m_visibleAurasToUpdate.insert(&other);AuraApplication::reads=0;
    reused.RemoveVisibleAura(&first);Require(reused.m_visibleAuras.size()==2&&reused.m_visibleAurasToUpdate.size()==2,"Late linked cleanup erased a same-slot replacement");Require(AuraApplication::reads==0,"Removal consulted an obsolete application's slot");
    // A raw queued pointer may outlive its application. Identity removal must
    // never dereference that storage, even when another aura has reused a slot.
    Unit queued;auto retired=new AuraApplication{0};queued.m_visibleAurasToUpdate.insert(retired);queued.m_visibleAurasToUpdate.insert(&other);delete retired;AuraApplication::reads=0;
    queued.RemoveVisibleAura(retired);Require(queued.m_visibleAurasToUpdate.size()==1&&*queued.m_visibleAurasToUpdate.begin()==&other&&AuraApplication::reads==0,"Pending retired application was dereferenced or retained");
    std::cout<<"PASS: extracted production identity removal preserves same-slot replacements and removes retired pending pointers without slot reads.\n";
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}return 0;}
