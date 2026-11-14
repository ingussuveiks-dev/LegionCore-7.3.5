"""Check the actual linked functional return hooks, native motion and transfer."""
from pathlib import Path
import runpy
import json
import hashlib
from sharasdal_erunak_entry_pe import sites, values

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))

def require(prefix, targets=(), constants=()):
    body = n['instructions'](prefix)
    for target in targets:
        assert sites(n, body, target), (prefix, target)
    assert set(constants) <= values(body), (prefix, constants, values(body))
    return body

require('?AddSC_instance_temple_of_the_jade_serpent@@', ('?Register@FuZanReturn@@',))
require('?Register@FuZanReturn@@', ('??0CreatureScript@@', '??0PlayerScript@@'))
require('?sGossipSelect@AI@ReturnActor@FuZanReturn@@', ('?Request@FuZanReturn@@',))
require('?OnUpdate@PlayerEvents@FuZanReturn@@', ('?Update@FuZanReturn@@',))
require('?PassengerBoarded@AI@ReturnVehicle@FuZanReturn@@', ('?Boarded@FuZanReturn@@',))
require('?IsSummonedBy@AI@ReturnVehicle@FuZanReturn@@', ('?RegisterVehicle@FuZanReturn@@',))
require('?MovementInform@AI@ReturnVehicle@FuZanReturn@@', constants=(0x465A5201,))
require('?Request@FuZanReturn@@', ('?GetQuestCancellationReceipt@Player@@', '?GetScenario@ScenarioMgr@@',
    '?IsWithinLOSInMap@WorldObject@@', '?GetItemByEntry@Player@@'), (109440, 128938, 19932, 1625, 1137))
require('?Update@FuZanReturn@@', ('?FlightClear@FuZanReturn@@', '?MoveSmoothFlyPath@MotionMaster@@',
    '?SetClientControl@Player@@', '?ExitVehicle@Unit@@', '?CastSpell@Unit@@'), (217818, 217820, 46598, 55000))
require('?Hit@spell_fuzan_return_boarding@FuZanReturn@@', ('?QuestOwned@FuZanReturn@@', '?PreventHitDefaultEffect@SpellScript@@'))
require('?Finished@spell_fuzan_return_boarding@FuZanReturn@@', ('?UpdateAchievementCriteria@Player@@',), (52751,))
require('?Hit@spell_fuzan_return_transfer@FuZanReturn@@', ('?GetHitDest@SpellScript@@', '?PreventHitDefaultEffect@SpellScript@@'))
require('?OnBeforeLogout@PlayerEvents@FuZanReturn@@', ('?Clear@FuZanReturn@@',))
print('PASS linked Fu Zan reconstructed departure: registered gossip/seat/motion/player hooks, native spell/event gates, collision, control restoration and before-save cleanup')
print(json.dumps({'world_sha256': hashlib.sha256(n['binary']).hexdigest(), 'status': 'FUNCTIONAL_RECONSTRUCTION_CLIENT_PENDING'}))
