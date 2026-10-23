"""Reject stale scene cleanup in the canonical linked executable."""
from pathlib import Path
import runpy

native = runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
erase = '?erase@?$_Hash@V?$_Umap_traits@IIV?$_Uhash_compare@IU?$hash@I@std@@U?$equal_to@I@2@@std@@V?$allocator@U?$pair@$$CBII@std@@@2@$0A@@std@@@std@@QEAA_KAEBI@Z'
for caller, callees in (
    ('?SceneCompleted@Player@@', ('?HasSceneStatus@Player@@', '?TrigerScene@Player@@', '?RemoveAurasDueToSpell@Unit@@', erase)),
    ('?SendSpellScene@Player@@', ('?SceneCompleted@Player@@',)),
    ('?HandleScenePlaybackCanceled@WorldSession@@', ('?SceneCompleted@Player@@',)),
    ('?HandleScenePlaybackComplete@WorldSession@@', ('?SceneCompleted@Player@@',)),
):
    targets = native['calls'](caller)
    for callee in callees:
        assert native['symbol'](callee) in targets, f'Deployed scene cleanup lacks {caller} -> {callee}'
print('PASS: linked scene completion/removal closes instance IDs, checks completion status and preserves native cancel/complete dispatch')
