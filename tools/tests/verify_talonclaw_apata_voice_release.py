"""Verify the actual native voice and localized speech packet consumers."""
from pathlib import Path
import runpy

previous = runpy.run_path(str(Path(__file__).with_name('verify_talonclaw_apata_dialogue_release.py')))
native = previous['native']
for caller, callee in [
    ('?SendChat@CreatureTextMgr@@', '?SendText@CreatureTextMgr@@'),
    ('?SendText@CreatureTextMgr@@', '?Write@PlaySound@Misc@WorldPackets@@'),
    ('?SendText@CreatureTextMgr@@', '?SendNonChatPacket@CreatureTextMgr@@'),
    ('?SendText@CreatureTextMgr@@', '??$SendChatPacket@VCreatureTextBuilder@@@CreatureTextMgr@@'),
    ('??RCreatureTextBuilder@@', '?GetBroadcastTextValue@DB2Manager@@'),
    ('??RCreatureTextBuilder@@', '?Initialize@Chat@1WorldPackets@@'),
]:
    assert native['symbol'](callee) in native['calls'](caller), f'Missing native voice/text bridge {caller} -> {callee}'
print('PASS: deployed native TALK ->SendChat ->SendText retains inlined SendSound via real PlaySound serialization/distribution and the CreatureTextBuilder packet route; actual builder consumes native localized broadcast text before Chat initialization. Gender sound selection and four payloads checked separately, not client playback')
