"""Check the claim SQL contract against the installed 7.3.5 DB2 data."""
import json
import sys
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/research"))
from audit_spell_mechanics import Data


def main():
    import os
    os.chdir(ROOT)
    data = Data(ROOT / "build-extractors/bin/Release/dbc/enUS", ROOT / "__native_only__")
    steps = sorted((r for r in data.rows("ScenarioStep").values() if r["ScenarioID"] == 998),
                   key=lambda r: r["OrderIndex"])
    assert [(r["ID"], r["Criteriatreeid"]) for r in steps] == [(2057, 46487), (2058, 46489), (2059, 46491)]
    trees, criteria = data.rows("CriteriaTree"), data.rows("Criteria")
    expected = [[(92, 48186)], [(36, 132880)], [(92, 48182), (92, 48187)]]
    for step, wanted in zip(steps, expected):
        children = [r for r in trees.values() if r["Parent"] == step["Criteriatreeid"]]
        assert all(r["Amount"] == 1 for r in children)
        actual = sorted((criteria[r["CriteriaID"]]["Type"], criteria[r["CriteriaID"]]["Asset"]) for r in children)
        assert actual == sorted(wanted), (step["ID"], actual)
    effects = data.rows("SpellEffect").values()
    assert any(r["SpellID"] == 200095 and r["Effect"] == 61 and r["EffectMiscValue1"] == 48173 for r in effects)
    assert any(r["SpellID"] == 201205 and r["Effect"] == 252 for r in effects)
    assert any(r["SpellID"] == 201205 and r["Effect"] == 134 and r["EffectMiscValue1"] == 101524 for r in effects)
    print(json.dumps({"result": "PASS", "scenario": 998, "stages": expected, "native_sha256": data.hashes}, indent=2))


if __name__ == "__main__":
    main()
