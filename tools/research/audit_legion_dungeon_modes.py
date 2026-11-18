"""Join matching native DB2, readonly World rows and real instance probe output.

Inputs contain public game metadata only. This is a structural audit, not a
substitute for combat, loot, key completion or client traversal.
"""
import argparse
import hashlib
import json
import math
from collections import Counter
from pathlib import Path


def rows(path):
    result = []
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        values = line.split("\t")
        if len(values) > 2 and values[1].isdigit():
            result.append(values)
    return result


def main():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument("--catalog", type=Path, required=True)
    parser.add_argument("--world", type=Path, required=True)
    parser.add_argument("--native-instance", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    catalog, world, native = map(rows, (args.catalog, args.world, args.native_instance))
    assert "PASS " in args.native_instance.read_text(encoding="utf-8-sig")
    maps = {int(r[1]): r[2] for r in catalog if r[0] == "MAP"}
    lfg = [r for r in catalog if r[0] == "LFG" and int(r[4]) <= 110 <= int(r[5])]
    selected = {int(r[1]) for r in lfg}
    modes = [r for r in catalog if r[0] == "MODE" and int(r[1]) in selected]
    expected = {(int(r[1]), int(r[3])) for r in modes}
    actual = {(int(r[1]), int(r[2])): r for r in native if r[0] == "INSTANCE"}
    assert len(selected) == 13 and len(expected) == 46 and set(actual) == expected
    templates = {int(r[1]): r[3] for r in world if r[0] == "INSTANCE"}
    assert set(templates) == selected
    for pair, r in actual.items():
        assert r[3] == templates[pair[0]] and int(r[4]) > 0 and int(r[5]) > 0
    encounters = [r for r in catalog if r[0] == "BOSS" and int(r[1]) in selected]
    credits = [r for r in world if r[0] == "ENCOUNTER"]
    assert {int(r[2]) for r in encounters} == {int(r[1]) for r in credits}
    assert len(encounters) == 63 and len(credits) == 66
    actors = {int(r[1]): r for r in world if r[0] == "CREATURE"}
    assert all(int(r[3]) == 0 and int(r[4]) in actors for r in credits)
    # Dantalionax is an entry transformation of the existing Latosius actor,
    # retaining its native AI; an empty separate factory is deliberate.
    assert {int(r[1]) for r in actors.values() if not r[3]} == {99611}
    entrances = {int(r[1]): r for r in world if r[0] == "ENTRANCE"}
    assert {int(r[2]) for r in lfg} <= set(entrances)
    assert all(all(math.isfinite(float(v)) for v in r[2:6]) for r in entrances.values())
    challenges = [r for r in catalog if r[0] == "CHALLENGE" and int(r[1]) in selected]
    starts = {(int(r[1]), int(r[2])) for r in native if r[0] == "START"}
    assert starts == {(int(r[1]), int(r[2])) for r in challenges} and len(starts) == 13
    assert len({map_id for map_id, _ in starts}) == 12
    assert all((map_id, 8) in expected for map_id, _ in starts)
    for r in modes:
        if int(r[3]) == 8:
            assert int(r[6]) == 23
    report = {
        "date": "2026-11-18", "client_build": 26972,
        "status": "PASS structural/native instance admission; whole gameplay OPEN",
        "selection": "Expansion6 MAP_INSTANCE with native LFG admission at level110; only actual native MapDifficulty modes. No raids.",
        "counts": {"maps": len(selected), "map_mode_pairs": len(expected),
                   "modes": dict(Counter(int(r[3]) for r in modes)),
                   "challenge_routes": len(starts), "challenge_maps": 12,
                   "native_encounters": len(encounters), "world_credit_rows": len(credits),
                   "lfg_entrances": len(entrances)},
        "maps": [{"id": map_id, "name": maps[map_id], "instance_script": templates[map_id],
                  "modes": sorted(mode for m, mode in expected if m == map_id),
                  "challenge_routes": sorted(route for m, route in starts if m == map_id),
                  "encounters": [{"id": int(r[2]), "name": r[5]} for r in encounters if int(r[1]) == map_id],
                  "gameplay": "OPEN"} for map_id in sorted(selected)],
        "excluded_native_maps": [{"id": map_id, "name": maps[map_id]} for map_id in sorted(set(maps) - selected)],
        "preserved_native_limits": [
            "Violet Hold has no Keystone mode.",
            "Arcway/Court of Stars/Karazhan/Cathedral/Seat have no native Normal mode admitting110.",
            "Karazhan has two distinct Keystone routes on one physical map.",
            "Seat quest47220/achievement12007 access gates were not bypassed.",
            "Dantalionax99611 retains Latosius AI through native UpdateEntry.",
        ],
        "remaining": ["Per-encounter/trash mechanics and failed/retry/group paths",
                      "Real combat/scaling/loot in each supported mode",
                      "Full Keystone start/affixes/forces/timer/completion/reward",
                      "Actual client entry, traversal, doors, visuals and reconnect"],
        "inputs": [{"file": p.name, "sha256": hashlib.sha256(p.read_bytes()).hexdigest()}
                   for p in (args.catalog, args.world, args.native_instance)],
    }
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print("PASS13 maps/46 mode pairs/13 challenge starts/63 encounter records/66 credit rows; structural audit only")


if __name__ == "__main__":
    main()
