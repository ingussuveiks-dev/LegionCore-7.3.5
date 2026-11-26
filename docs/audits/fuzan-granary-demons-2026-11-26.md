# Imperial Granary demon selection

The player reported that the demons could first be engaged with an area attack. The screenshot taken during combat shows a PvP-marked target. Read-only world data identified faction template 2780 on all 39 placements of entries 109021 (29), 109100 (4), 109106 (1) and 109140 (5). All placements use map 870, area 5981 and phase 6826. Creature initialization derives the PvP flag from the faction template. The Legion counterpart 2898 retains faction 1786 and hostility without the PvP flag.

Migration 2026_11_26_30 changes only those four templates with the expected old faction and placement scope. The running server still had the old faction during review. An actual target selection and single-target attack before area damage remain the client checks. No separate runtime test or startup was performed during active gameplay.
