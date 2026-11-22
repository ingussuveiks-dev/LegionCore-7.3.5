"""Full kill-credit helper, original file reversal, native catalog lookup."""
from pathlib import Path
import subprocess,sys
from prepare_core_delayed_owners import block,ROOT
BASE='cc4eb0ea7'; RECENT='c826f0dc2'; PATH='src/server/game/Entities/Player/KillRewarder.cpp'
OLD='''            ObjectGuid victimGuid = creature->GetGUID();
            player->AddDelayedEvent(50, [=]() -> void
            {
                if (Creature* target = ObjectAccessor::GetCreature(*player, victimGuid))
                {
                    player->KilledMonster(target->GetCreatureTemplate(), victimGuid);
                    target->AddRewardPlayer(player->GetGUID());
                }
            });'''
CONFIRMED='''            ObjectGuid victimGuid = creature->GetGUID();
            uint32 victimEntry = creature->GetEntry();
            player->AddDelayedEvent(50, [=]() -> void
            {
                // Death credit belongs to this confirmed kill. Its corpse may
                // already have left the map before the player queue is pumped.
                if (CreatureTemplate const* victimTemplate = sObjectMgr->GetCreatureTemplate(victimEntry))
                    player->KilledMonster(victimTemplate, victimGuid);

                if (Creature* target = ObjectAccessor::GetCreature(*player, victimGuid))
                    target->AddRewardPlayer(player->GetGUID());
            });'''
NEW='''            ObjectGuid victimGuid = creature->GetGUID();
            uint32 victimEntry = creature->GetEntry();
            auto sfgReceipt = _killingSpellId == 69743 && victimEntry == 36868 &&
                player->GetQuestStatus(14472) == QUEST_STATUS_INCOMPLETE ? player->GetQuestCancellationReceipt(14472) : nullptr;
            player->AddDelayedEvent(50, [=]() -> void
            {
                // Death credit belongs to this confirmed kill. Its corpse may
                // already have left the map before the player queue is pumped.
                if (CreatureTemplate const* victimTemplate = sObjectMgr->GetCreatureTemplate(victimEntry))
                    player->KilledMonster(victimTemplate, victimGuid,
                        sfgReceipt && !sfgReceipt->load(std::memory_order_acquire) ? 69743 : 0);

                if (Creature* target = ObjectAccessor::GetCreature(*player, victimGuid))
                    target->AddRewardPlayer(player->GetGUID());
            });'''
def verify():
    old=subprocess.check_output(['git','-C',str(ROOT),'show',BASE+':'+PATH]).decode('utf-8-sig').replace('\r\n','\n');new=(ROOT/PATH).read_text()
    assert old.count(OLD)==1
    expected=subprocess.check_output(['git','-C',str(ROOT),'show',RECENT+':'+PATH]).decode('utf-8-sig').replace('\r\n','\n')
    assert expected.count(CONFIRMED)==1
    expected=expected.replace('bool isBattleGround, GuidSet* darkmoonRecipients)','bool isBattleGround, GuidSet* darkmoonRecipients, uint32 killingSpellId)',1)
    expected=expected.replace('    _darkmoonRecipients = darkmoonRecipients;','    _darkmoonRecipients = darkmoonRecipients;\n    _killingSpellId = killingSpellId;',1)
    assert expected.replace(CONFIRMED,NEW,1)==new,'undeclared reward/owner/group/XP change'
    marker='inline void KillRewarder::_RewardKillCredit('
    return block(old,marker),block(new,marker)
if __name__=='__main__':
    old,new=verify();out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True);(out/'Reward.inc').write_text(old if len(sys.argv)>2 and sys.argv[2]=='baseline' else new)
    player=(ROOT/'src/server/game/Entities/Player/Player.cpp').read_text()
    signature='void Player::KilledMonster(CreatureTemplate const* cInfo, ObjectGuid guid'
    (out/'KilledMonster.inc').write_text(block(player,signature+')')+'\n'+block(player,signature+', uint32 killingSpellId)'))
    (out/'FunctionProcessorActual.inc').write_text((ROOT/'src/common/Utilities/FunctionProcessor.cpp').read_text())
    print('PASS exact delayed-credit scope: confirmed victim entry/native template survives corpse loss,50ms/native group guard/live reward-mark retained;full file pinned,other rewards unchanged')
