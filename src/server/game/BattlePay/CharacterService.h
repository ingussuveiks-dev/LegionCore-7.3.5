
#ifndef _sCharService
#define _sCharService

#include "ObjectGuid.h"

class Player;
class WorldSession;

class TC_GAME_API CharacterService
{
	CharacterService() = default;
	~CharacterService() = default;
	
public:
    void SetRename(Player* player);
    void ChangeFaction(Player* player);
    void ChangeRace(Player* player);
    void Customize(Player* player);
    bool Boost(Player* player, uint16 specializationId, uint8 targetLevel);
    std::vector<uint32> GetBoostItems(Player const* player, uint16 specializationId, uint8 targetLevel) const;
    std::vector<uint32> GetBoostItems(uint8 classId, uint16 specializationId, uint8 targetLevel) const;
    bool BoostCharacter(WorldSession* session, ObjectGuid targetCharGuid, uint16 specializationId, uint8 targetLevel,
        std::vector<uint32>& boostItems);
    void RestoreDeletedCharacter(WorldSession* session);

	static CharacterService* instance();
};

#define sCharacterService CharacterService::instance()

#endif
