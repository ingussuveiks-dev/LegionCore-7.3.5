"""Compare the audit against outcomes of extracted native LINK execution."""
import argparse
from pathlib import Path
import sys
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'research'))
from artifact_smart_links import invalid_links

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--native', type=Path, required=True)
a = p.parse_args()
count = 0
for line in a.native.read_text().splitlines():
    name, event, action, link, target, expected = line.split('\t')
    group = [dict(entryorguid=105333, id=2, link=int(link), event_type=62, action_type=33)]
    if int(target):
        group.append(dict(entryorguid=105333, id=1, link=0, event_type=int(event), action_type=int(action)))
    findings = invalid_links(group)
    assert bool(findings) == bool(int(expected)), (name, findings, expected)
    if name == 'hello':
        # The old audit checked only membership: it missed this native refusal.
        old = [r for r in group if r['link'] and r['link'] not in {x['id'] for x in group}]
        assert not old and findings[0]['reason'] == 'invalid_target_event'
    count += 1
assert count == 7
print('PASS seven extracted-native LINK outcomes; old missing-ID-only audit misses the real Gossip Hello counterexample')
