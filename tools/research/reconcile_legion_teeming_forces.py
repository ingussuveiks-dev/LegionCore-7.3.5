"""Reconcile pinned MDT counts with native weighted criteria, retaining every leaf.

Read-only inputs from Read-LegionTeemingMapCatalog and the native route probe.
This checks metadata, not earned kills or original server spawn placements.
"""
import argparse
import collections
import hashlib
import json
from pathlib import Path
import re


def reconcile(catalog, route_log, source, addon_directory):
    trees = {}
    for line in catalog.read_text(encoding="utf-8-sig").splitlines():
        row = line.split("\t")
        if row[0] == "TREE":
            key = int(row[1])
            assert key not in trees, f"Duplicate native tree ID {key}"
            trees[key] = dict(id=key, parent=int(row[2]), amount=int(row[3]),
                              operator=int(row[4]), criterion=int(row[5]),
                              type=int(row[6]), asset=int(row[7]), modifier=int(row[8]))
    routes = {int(route): int(tree) for route, tree in re.findall(
        r"^ROUTE id=(\d+) map=\d+ teeming=1 .*? expected=(\d+)",
        route_log.read_text(encoding="utf-8-sig"), re.M)}

    def descendants(root):
        selected = {root}
        while True:
            expanded = selected | {key for key, row in trees.items() if row["parent"] in selected}
            if expanded == selected:
                return selected
            selected = expanded

    results = []
    for route in source["routes"]:
        path = addon_directory / route["source_file"]
        raw = path.read_bytes()
        assert hashlib.sha256(raw).hexdigest() == route["source_sha256"], "Pinned addon changed"
        counts = {}
        npc = None
        for line in raw.decode("utf-8-sig").splitlines():
            match = re.search(r'\["id"\]\s*=\s*(\d+)', line)
            if match:
                npc = int(match[1])
            match = re.search(r'\["count"\]\s*=\s*(\d+)', line)
            if match:
                value = int(match[1])
                assert npc not in counts or counts[npc] == value, f"Conflicting addon NPC counts {npc}"
                counts[npc] = value
        selected = routes[route["challenge_id"]]
        weighted, = [key for key in descendants(selected) if trees[key]["operator"] == 9]
        required = trees[weighted]["amount"]
        assert required == route["addon_teeming_forces"], "Native/addon force target differs"
        by_npc = collections.defaultdict(list)
        for key in sorted(descendants(weighted) - {weighted}):
            row = trees[key]
            # Fail explicitly on unsupported operators or admission predicates.
            assert row["operator"] == 0 and row["type"] == 0 and row["modifier"] == 0, row
            assert row["parent"] == weighted and row["criterion"] and row["amount"] > 0, row
            by_npc[row["asset"]].append(row)
        comparisons = []
        for npc in sorted({point["npc_id"] for point in route["points"]}):
            leaves = by_npc[npc]
            assert leaves, f"Missing native NPC weight {npc}"
            effective = sum(leaf["amount"] for leaf in leaves)
            comparisons.append(dict(npc_id=npc, addon_weight=counts[npc],
                                    native_weights=[leaf["amount"] for leaf in leaves],
                                    native_leaf_ids=[leaf["id"] for leaf in leaves],
                                    native_criteria_ids=[leaf["criterion"] for leaf in leaves],
                                    native_effective_weight=effective,
                                    metadata_match=counts[npc] == effective))
        results.append(dict(challenge_id=route["challenge_id"], map_id=route["map_id"],
                            native_teeming_tree=selected, native_forces_tree=weighted,
                            native_required=required, addon_required=route["addon_teeming_forces"],
                            target_match=True, extra_npc_weight_metadata=comparisons))
    return results


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", type=Path, required=True)
    parser.add_argument("--route-log", type=Path, required=True)
    parser.add_argument("--source-catalog", type=Path, required=True)
    parser.add_argument("--addon-directory", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source = json.loads(args.source_catalog.read_text(encoding="utf-8-sig"))
    result = reconcile(args.catalog, args.route_log, source, args.addon_directory)
    rows = [row for route in result for row in route["extra_npc_weight_metadata"]]
    assert len(result) == 13 and len(rows) == 106
    assert all(row["metadata_match"] for row in rows), "Unresolved native/addon weight mismatch"
    args.output.write_bytes((json.dumps(result, indent=2) + "\n").encode("utf-8"))
    print(f"PASS {len(result)} native force targets and {len(rows)} NPC weights; distinct weighted leaves retained")
