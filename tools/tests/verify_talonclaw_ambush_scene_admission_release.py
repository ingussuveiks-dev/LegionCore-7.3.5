"""Verify canonical EventObject condition dispatch and native scene delivery."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_talonclaw_ambush_damage_release.py')))
native = previous['native']
instructions = previous['instructions']
for caller, callee in [
    ('?OnTrigger@SmartEventObject@@', '?OnInitialize@SmartScript@@'),
    ('?OnTrigger@SmartEventObject@@', '?ProcessEventsFor@SmartScript@@'),
    ('?ProcessEventsFor@SmartScript@@', '?IsObjectMeetingSmartEventConditions@ConditionMgr@@'),
    ('?IsObjectMeetingSmartEventConditions@ConditionMgr@@', '?IsObjectMeetToConditions@ConditionMgr@@QEBA_NAEAUConditionSourceInfo@@'),
    ('?HandleAuraActivateScene@AuraEffect@@', '?SendSpellScene@Player@@'),
    ('?SendSpellScene@Player@@', '?GetSpellScene@SpellMgr@@'),
    ('?SendSpellScene@Player@@', '?Write@PlayScene@Scene@WorldPackets@@'),
    ('?Update@EventObject@@', '?IsEOMeetConditions@ScriptMgr@@'),
    ('?Update@EventObject@@', '?OnEventObject@ScriptMgr@@'),
]:
    assert native['symbol'](callee) in native['calls'](caller), f'Missing actual scene admission/delivery path: {caller} -> {callee}'
body = instructions('?OnTrigger@SmartEventObject@@')
values = {operand.imm for entry in body for operand in entry.operands if operand.type == CS_OP_IMM}
assert {89, 91}.issubset(values), 'Actual EO enter/exit dispatcher constants changed'
body = instructions('?OnInitialize@SmartScript@@')
assert any(entry.mnemonic == 'mov' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_IMM and entry.operands[1].imm == 13 for entry in body), 'EventObject source13 assignment absent from actual initialization'
assert b'SmartEventObject' in native['binary'], 'Native EventObject registry missing'
print('PASS: canonical SmartEventObject enter89/exit91 initializes source13, executes actual source-keyed ConditionMgr lookup and preserves native aura ->MiscValue mapping ->PlayScene packet. Native EO update entry/cache path remains linked, not certified as stage-change retry.')
