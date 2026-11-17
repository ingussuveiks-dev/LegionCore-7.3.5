#pragma once
#include <type_traits>
#include <utility>

// Controlled inventory dependency for existing actor/scene host tests. The
// real classification, ownership, bank and linked-child rules are executed
// separately by Test-ArtifactAcquisitionInventory against the native catalog.
// These hosts exercise their production controllers with the requested outcome.
namespace ArtifactAcquisitionInventory
{
template<class P, class = void> struct OtherSpec { static bool Get(P const*) { return false; } };
template<class P> struct OtherSpec<P, std::void_t<decltype(std::declval<P>().preserveOffSpec)>>
{ static bool Get(P const* player) { return player && player->preserveOffSpec; } };
template<class P, class I> bool OtherSpecialization(P* player, I const*) { return OtherSpec<P>::Get(player); }
template<class P, class I> bool StoredForOtherSpecialization(P* player, I* item, unsigned = 0)
{ return player && item && OtherSpec<P>::Get(player); }
template<class I> auto Entry(I* item, int) -> decltype(item->GetEntry()) { return item->GetEntry(); }
template<class I> auto Entry(I* item, long) -> decltype(item->entry) { return item->entry; }
template<class G> auto Same(G const& a, G const& b, int) -> decltype(a == b) { return a == b; }
template<class G> auto Same(G const& a, G const& b, long) -> decltype(a != b) { return !(a != b); }
template<class I, class = void> struct Pair
{ static bool Ready(I*, I*, unsigned) { return false; } };
template<class I> struct Pair<I, std::void_t<decltype(std::declval<I>().GetChildItem()),decltype(std::declval<I>().GetGUID())>>
{
    static bool Ready(I* parent, I* child, unsigned entry)
    { return child && Entry(child,0)==entry && Same(parent->GetChildItem(),child->GetGUID(),0); }
};
template<class P> bool Ready(P* player, unsigned entry, unsigned slot = 15, unsigned child = 0, unsigned childSlot = 16)
{
    auto parent = player ? player->GetItemByEntry(entry) : nullptr;
    if (!parent) return false;
    if (OtherSpec<P>::Get(player)) return true;
    if (player->GetItemByPos(255,slot)!=parent) return false;
    return !child || Pair<std::remove_pointer_t<decltype(parent)>>::Ready(parent,player->GetItemByPos(255,childSlot),child);
}
}
