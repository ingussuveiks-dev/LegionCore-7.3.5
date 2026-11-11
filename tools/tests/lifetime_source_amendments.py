"""Explicit later payload repair;historical lifetime proofs retain their baseline.

The coordinate repair has its own full-method native geometry regression.
Normalize only this exact amendment when checking the older capture-only diff;
the old/new lifetime plan and its pinned controls are never rewritten.
"""
CORDANA_PATH='src/server/scripts/Legion/VaultoftheWardens/boss_cordana.cpp'
CORDANA_OLD='for(x = creeping_walls[current_wall].first.GetPositionX(); y > creeping_walls[current_wall].second.GetPositionX(); x -= 2.5f)'
CORDANA_NEW=CORDANA_OLD.replace('; y >','; x >')

def lifetime_baseline_view(path,source):
    from five_ten_20261111_scope import retained_source
    source=retained_source(path,source)
    if path.replace('\\','/')==CORDANA_PATH:
        assert source.count(CORDANA_NEW)<=1
        source=source.replace(CORDANA_NEW,CORDANA_OLD)
    return source
