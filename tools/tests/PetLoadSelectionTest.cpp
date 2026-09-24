#include "PetDefines.h"
#include <cassert>
#include <utility>

// The runner extracts the production function, so this exercises its actual
// lookup rather than a second implementation of the selection rules.
class Pet
{
public:
    static std::pair<PetStable::PetInfo const*, PetSaveMode> GetLoadPetInfo(
        PetStable const&, uint32, uint32, Optional<PetSaveMode>);
};
#include "PetLoadSelection.inc"

int main()
{
    PetStable stable;
    PetStable::PetInfo imp;
    imp.PetNumber = 7;
    imp.CreatureId = 416;
    PetStable::PetInfo voidwalker;
    voidwalker.PetNumber = 5;
    voidwalker.CreatureId = 1860;
    stable.UnslottedPets = {imp, voidwalker};
    stable.SetCurrentUnslottedPetIndex(1);

    // Vehicle exit requests entry 0 and the saved unique pet number.
    auto result = Pet::GetLoadPetInfo(stable, 0, 5, {});
    assert(result.first == &stable.UnslottedPets[1]);
    assert(result.second == PET_SAVE_NOT_IN_SLOT);
    assert(Pet::GetLoadPetInfo(stable, 0, 7, {}).first == &stable.UnslottedPets[0]);
    // A number lookup must not substitute another demon, even if entry matches.
    assert(!Pet::GetLoadPetInfo(stable, 416, 99, {}).first);
    assert(Pet::GetLoadPetInfo(stable, 1860, 0, {}).first == &stable.UnslottedPets[1]);

    // Hunter active-slot lookup must continue to work.
    stable.ActivePets[2] = voidwalker;
    stable.UnslottedPets.clear();
    assert(Pet::GetLoadPetInfo(stable, 0, 5, {}).first == &*stable.ActivePets[2]);
}
