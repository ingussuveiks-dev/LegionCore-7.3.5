"""Verify the original Legion invisibility, encounter and pet item data."""

import argparse
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "research"))
from audit_spell_visuals import Table


def verify(dbc):
    effects = Table(dbc / "SpellEffect.db2", index_field=0)
    expected = {
        100512: (6, 100, 18, [14, 0]),
        100617: (6, 100, 18, [15, 0]),
        100618: (6, 101, 19, [14, 0]),
        100619: (6, 101, 19, [15, 0]),
        100668: (28, 0, 0, [54114, 2926]),
        100684: (28, 1, 0, [54128, 3221]),
    }
    actual = {
        spell: tuple(effects.value(record, field) for field in (1, 2, 4, 26))
        for record, spell in effects.parents.items()
        if spell in expected and effects.value(record, 3) == 0 and effects.value(record, 5) == 0
    }
    if expected != actual:
        raise ValueError(f"Unexpected Legion spell records {actual!r}")

    explosion = [tuple(effects.value(record, field) for field in (1, 3, 5, 16, 27, 28))
                 for record, spell in effects.parents.items() if spell == 100669]
    if explosion != [(140, 0, 0, 100668, [0, 9], [22, 7])]:
        raise ValueError(f"Unexpected native Void Explosion {explosion!r}")
    lightning = [tuple(effects.value(record, field) for field in (1, 3, 4, 5, 28))
                 for record, spell in effects.parents.items() if spell == 100922]
    visuals = Table(dbc / "SpellXSpellVisual.db2", index_field=0)
    if lightning != [(6, 0, 4, 0, [38, 0])] or not any(
        spell == 100922 and visuals.value(record, 1) == 48545 for record, spell in visuals.parents.items()
    ):
        raise ValueError("Unexpected native Sanath's Lightning aura or visual")

    for spell in (100671, 100792):
        ritual = {
            effects.value(record, 3): tuple(effects.value(record, field) for field in (1, 4, 26, 28))
            for record, parent in effects.parents.items() if parent == spell and effects.value(record, 5) == 0
        }
        if ritual != {0: (6, 5, [0, 0], [1, 0]), 1: (6, 31, [0, 0], [1, 0]), 2: (6, 56, [43, 1], [1, 0])}:
            raise ValueError(f"Unexpected native ritual for spell {spell}")

    item_effects = Table(dbc / "ItemEffect.db2")
    crate_spells = [item_effects.value(record, 0) for record, item in item_effects.parents.items() if item == 71076]
    if sorted(crate_spells) != [55884, 100684] or 71057 in item_effects.parents.values():
        raise ValueError("Unexpected use spells for the quest crate or companion item")
    print("PASS Legion spider visibility, Void Explosion and its native summon, both spider rituals and the crate item spells")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dbc", type=Path, default=Path("build-extractors/bin/Release/dbc/enUS"))
    verify(parser.parse_args().dbc)
