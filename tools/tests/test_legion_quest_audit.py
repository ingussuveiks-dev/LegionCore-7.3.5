"""Regression tests for graph diagnostics, not a game simulation."""
import sys
import unittest
from pathlib import Path
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'research'))
from audit_legion_quests import predecessors, reachable


def row(prev=0, nxt=0):
    return dict(PrevQuestID=prev, NextQuestID=nxt)


class GraphTests(unittest.TestCase):
    def test_gearing_up_and_successor_prerequisite(self):
        quests = {34314, 34315, 34316}
        addons = {34315: row(34315, 34315)}
        self.assertNotIn(34315, reachable(quests, set(), predecessors(quests, addons)))
        addons[34315] = row(34314, 34316)
        prev = predecessors(quests, addons)
        self.assertEqual(prev[34315], {34314})
        self.assertEqual(prev[34316], {34315})
        self.assertEqual(reachable(quests, set(), prev), quests)
        self.assertEqual(reachable(quests, {34314}, prev), set())

    def test_archaeology_chain(self):
        quests = {41183, 41184, 41185}
        addons = {41183: row(nxt=41183), 41184: row(41183, 41185), 41185: row(41184)}
        self.assertEqual(reachable(quests, set(), predecessors(quests, addons)), set())
        addons[41183]['NextQuestID'] = 41184
        prev = predecessors(quests, addons)
        self.assertEqual(prev, {41183: set(), 41184: {41183}, 41185: {41184}})
        self.assertEqual(reachable(quests, set(), prev), quests)

    def test_external_next_edge_opens_cycle(self):
        quests = {1, 2, 3}
        prev = predecessors(quests, {1: row(nxt=2), 2: row(3), 3: row(2)})
        self.assertEqual(reachable(quests, set(), prev), quests)

    def test_disabled_root_and_negative_active_prerequisite(self):
        quests = {1, 2, 3}
        prev = predecessors(quests, {1: row(nxt=-2), 3: row(-2)})
        self.assertEqual(prev[2], {-1})
        self.assertEqual(reachable(quests, {1}, prev), set())
        self.assertEqual(reachable(quests, set(), prev), quests)


if __name__ == '__main__':
    unittest.main()
