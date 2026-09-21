
#include "ScriptMgr.h"
#include "BattlePayMgr.h"
#include "BattlePayData.h"
#include "Chat.h"
#include "Player.h"

#include <cstdlib>
#include <cstring>

class battlepay_commandscript : public CommandScript
{
public:
    battlepay_commandscript() : CommandScript("battlepay_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> BattlepayCommandTable =
        {
            { "add",                    SEC_ADMINISTRATOR,  false,  &HandleAddBattlePayTokens,       "Usage: .battlepay add <amount> [tokenType]. Adds tokens to the selected player or yourself."},
            { "balance",                SEC_ADMINISTRATOR,  false,  &HandleBattlePayBalance,         "Usage: .battlepay balance [tokenType]. Shows the selected player or your own balance."},
            { "reload",                 SEC_ADMINISTRATOR,  false,  &HandleReloadBattlePay,          "Reloads the BattlePay catalog from the world database."}
        };

        static std::vector<ChatCommand> CommandTable =
        {
             { "battlepay",             SEC_ADMINISTRATOR,  true,   nullptr,                        "", BattlepayCommandTable }
        };

        return CommandTable;
    }

    static bool ExtractTokenType(char const* value, uint8& tokenType)
    {
        tokenType = 1;
        if (!value)
            return true;

        char* end = nullptr;
        unsigned long parsed = std::strtoul(value, &end, 10);
        if (!end || *end != '\0' || parsed == 0 || parsed > 255)
            return false;

        tokenType = uint8(parsed);
        return sBattlePayDataStore->GetTokenTypes().count(tokenType) != 0;
    }

    static bool HandleAddBattlePayTokens(ChatHandler* handler, char const* args)
    {
        if (!args || !*args)
            return false;

        char* amountText = std::strtok(const_cast<char*>(args), " ");
        char* tokenText = std::strtok(nullptr, " ");

        char* end = nullptr;
        unsigned long long amount = std::strtoull(amountText, &end, 10);
        if (!end || *end != '\0' || amount == 0 || amount > 1000000000ULL)
        {
            handler->SendSysMessage("The amount must be between 1 and 1000000000.");
            return false;
        }

        uint8 tokenType;
        if (!ExtractTokenType(tokenText, tokenType))
        {
            handler->SendSysMessage("Unknown BattlePay token type.");
            return false;
        }

        Player* target = handler->getSelectedPlayer();
        if (!target || !target->GetSession())
        {
            handler->SendSysMessage("Select an online player first.");
            return false;
        }

        if (!target->ChangeTokenCount(tokenType, int64(amount), Battlepay::BattlepayCustomType::BattlePayShop, 0))
            return false;

        handler->PSendSysMessage("Added %llu %s to %s. New balance: %lld.", amount,
            sBattlePayDataStore->GetTokenTypes()[tokenType].name.c_str(), target->GetName(),
            target->GetSession()->GetTokenBalance(tokenType));
        return true;
    }

    static bool HandleBattlePayBalance(ChatHandler* handler, char const* args)
    {
        uint8 tokenType;
        if (!ExtractTokenType(args && *args ? args : nullptr, tokenType))
        {
            handler->SendSysMessage("Unknown BattlePay token type.");
            return false;
        }

        Player* target = handler->getSelectedPlayer();
        if (!target || !target->GetSession())
        {
            handler->SendSysMessage("Select an online player first.");
            return false;
        }

        handler->PSendSysMessage("%s has %lld %s.", target->GetName(),
            target->GetSession()->GetTokenBalance(tokenType), sBattlePayDataStore->GetTokenTypes()[tokenType].name.c_str());
        return true;
    }

    static bool HandleReloadBattlePay(ChatHandler* chatHandler, char const* /*args*/)
    {
        sBattlePayDataStore->Initialize();
        chatHandler->SendSysMessage("BattlePay catalog reloaded.");
        return true;
    }
};

void AddSC_battlepay_commandscript()
{
    new battlepay_commandscript();
}
