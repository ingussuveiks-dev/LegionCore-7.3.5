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
    if name.startswith('timed-owner-'):
        timed = [dict(entryorguid=10533300,id=2,link=1,event_type=0,action_type=33),
                 dict(entryorguid=10533300,id=1,link=0,event_type=64 if name.endswith('valid') else 61,action_type=33)]
        findings = invalid_links(timed, owner_events=group[1:])
        assert bool(invalid_links(timed)) != bool(findings), 'Old same-list lookup must disagree on all three counterexamples'
    else:
        findings = invalid_links(group)
    assert bool(findings) == bool(int(expected)), (name, findings, expected)
    if name == 'hello':
        # The old audit checked only membership: it missed this native refusal.
        old = [r for r in group if r['link'] and r['link'] not in {x['id'] for x in group}]
        assert not old and findings[0]['reason'] == 'invalid_target_event'
    count += 1
assert count == 10
print('PASS ten extracted-native LINK outcomes, including three source9/owner-event counterexamples; original seven primary-event outcomes retained')
