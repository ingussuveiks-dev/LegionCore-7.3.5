# Freezing Roots credit

The player confirmed Frost Nova appeared on button seven but the scenario stayed at 0/1. Scenario 1132 tree 48840 expects script event 52062. The registered boost dummy 107104 handled damage and had no handler for that event. The local template has zero mechanic immunity mask. The build 26972 spell data for 122 has area damage on effect zero and an eight-second root aura (455) on effect one.

The ship dummy now checks the hitting spell and its applied root effect from the actual player, the shared transport, tutorial map, scenario and current tree before reporting event 52062 to that player. A pet Freeze or a Frost Nova cast outside the dummy’s radius supplies no such receipt. Later hits after the lesson changes do not issue credit again.

The Release link used the original scripts.lib and resolves npc_boost_training_dummy::SpellHit to scripts:boost_experience.obj. Promoted with worldserver stopped. Verified SHA-256 F97723B98061935A4AE6AD32AB564F8166C835C54ABEF9164558BC8143C1D750 (EXE) and 4EA49234E60BA33DD8D07C00B020EC73EDCE51A51A487E5518E08C05185C2E89 (map). Canonical crash directory and DBErrors.log were empty. No separate automated tutorial test or agent-owned server startup was run during active client traversal. Counter advancement and transition to Blizzard await client confirmation.
