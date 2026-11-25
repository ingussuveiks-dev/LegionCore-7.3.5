# Orgrimmar funeral pyre cell activation

The server continued to record Black Arrow (225289) ticks from Dark Ranger Kalira (350986) and Dark Ranger Velonara (351012) on the Felblade Assassin (351078) after the last player logged out. The NPC combat damage floor kept the assassin alive, so the staged fight repeated while the cell was updated.

The live MariaDB data placed all eight NPCs and the funeral pyre (entry 258064, spawn 131298) in cell (274, 190) on map 1. The pyre is a phaseable world model with `MaxVisible=0`, spawn `isActive=0`, and phase IDs 7531, 7422, 5571, 2366, 2361, 2360, 2359, and 2284. The loader sets the spawn GUID before calling `GameObject::Create`. That function activated every phaseable model. An active object keeps its own cell in the map's update set even with an activation range of zero.

The repair skips forced activation for that exact map, entry, and spawn when maximum visibility is off. The model retains world-object registration, phase handling, and collision. All spell diagnostics and NPC combat mechanics continue to apply when the cell is updated.

## Verification

- The compiled creation-case regression reproduced forced activation from the pre-fix source and verified that the repaired case leaves the local pyre inactive. It checked world-object status in all six cases and retained activation for maximum visibility, other spawns, summoned copies, other entries, and other maps.
- The existing compiled game-object phase-mask regression passed. Neither test generated new crashes, server warnings or errors, or database diagnostics.
- Client checks are still open for the pyre's appearance and the staged fight when approaching or leaving the area during the funeral quest.
