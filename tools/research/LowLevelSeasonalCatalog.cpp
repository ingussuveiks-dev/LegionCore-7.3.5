#include "DB2LoadInfo.h"
#include "DB2Store.h"
#include "DB2Structure.h"

#include <iostream>
#include <string_view>

int main(int argc, char** argv)
{
    if (argc != 2)
        return 1;

    DB2Storage<ItemSparseEntry> items("ItemSparse.db2", ItemSparseLoadInfo::Instance());
    DB2Storage<HolidayNamesEntry> holidayNames("HolidayNames.db2", HolidayNamesLoadInfo::Instance());
    DB2Storage<HolidaysEntry> holidays("Holidays.db2", HolidaysLoadInfo::Instance());
    DB2Storage<MailTemplateEntry> mail("MailTemplate.db2", MailTemplateLoadInfo::Instance());
    DB2Storage<BroadcastTextEntry> broadcast("BroadcastText.db2", BroadcastTextLoadInfo::Instance());
    DB2Storage<BattlePetSpeciesEntry> species("BattlePetSpecies.db2", BattlePetSpeciesLoadInfo::Instance());
    DB2Storage<BattlePetSpeciesXAbilityEntry> speciesAbilities("BattlePetSpeciesXAbility.db2", BattlePetSpeciesXAbilityLoadInfo::Instance());
    DB2Storage<BattlePetAbilityEntry> abilities("BattlePetAbility.db2", BattlePetAbilityLoadInfo::Instance());
    if (!items.Load(argv[1],0) || !holidayNames.Load(argv[1],0) || !holidays.Load(argv[1],0) ||
        !mail.Load(argv[1],0) || !broadcast.Load(argv[1],0) || !species.Load(argv[1],0) || !speciesAbilities.Load(argv[1],0) || !abilities.Load(argv[1],0))
        return 2;

    for (auto row : items)
    {
        std::string_view name = row->Display->Str[0];
        if (row->StartQuestID == 47251 || row->StartQuestID == 47252 || row->StartQuestID == 43471 || row->StartQuestID == 43472 ||
            name.find("Anniversary Gift") != name.npos || name == "Celebration Package")
            std::cout << "ITEM\t" << row->ID << '\t' << name << '\t' << row->StartQuestID << '\n';
    }
    for (auto row : holidays)
    {
        auto name = holidayNames.LookupEntry(row->HolidayNameID);
        if (!name || std::string_view(name->Name->Str[0]).find("Anniversary") == std::string_view::npos)
            continue;

        std::cout << "HOLIDAY\t" << row->ID << '\t' << name->Name->Str[0] << '\t' << unsigned(row->Looping) << '\t' << row->Region;
        for (auto duration : row->Duration)
            std::cout << '\t' << duration;
        for (auto date : row->Date)
            std::cout << '\t' << uint32(date);
        std::cout << '\n';
    }
    for (auto row : mail)
    {
        std::string_view body = row->Body->Str[0];
        if (body.find("anniversary") != body.npos || body.find("Anniversary") != body.npos)
            std::cout << "MAIL\t" << row->ID << '\n';
    }
    for (auto row : broadcast)
    {
        std::string_view question = row->Text->Str[0];
        if (question.find("test your knowledge of history") == question.npos)
            continue;
        std::cout << "TRIVIA\t" << row->ID << '\t' << question << '\n';
        for (int i = 1; i <= 4; ++i)
            if (auto answer = broadcast.LookupEntry(row->ID + i))
                std::cout << "ANSWER\t" << answer->ID << '\t' << answer->Text->Str[0] << '\n';
    }
    for (auto row : species)
        if (row->ID >= 2066 && row->ID <= 2074)
        {
            std::cout << "SPECIES\t" << row->ID << '\t' << row->CreatureID << '\t' << unsigned(row->PetTypeEnum) << '\n';
            for (auto slot : speciesAbilities)
                if (slot->BattlePetSpeciesID == row->ID)
                {
                    auto ability = abilities.LookupEntry(slot->BattlePetAbilityID);
                    std::cout << "ABILITY\t" << slot->BattlePetSpeciesID << '\t' << slot->BattlePetAbilityID << '\t'
                        << int(slot->SlotEnum) << '\t' << unsigned(slot->RequiredLevel) << '\t'
                        << (ability ? ability->Name->Str[0] : "") << '\n';
                }
        }
    return 0;
}
