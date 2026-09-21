
#ifndef _sCharService
#define _sCharService

class Player;

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
    void RestoreDeletedCharacter(WorldSession* session);

	static CharacterService* instance();
};

#define sCharacterService CharacterService::instance()

#endif
