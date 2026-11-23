# Broken Shore Wyrmtongue container loot

The user opened empty Veiled Wyrmtongue Chests in Shadow Fracture near Malorus the Soulkeeper (spawn 140909) and in Broken Valley. Both use gathering-node entry 271554. The worldserver logged missing gameobject loot ID 72558. The entry's Data1 was 72558, while its 72 existing loot rows use 271554. Hidden Wyrmtongue Cache 271227 independently had Data1 set to empty ID 71869, while its 33 rows use 271227.

Migration 2026_11_23_09 points each type-50 object to its existing gameobject loot. It requires the old ID to be empty and the target table to have rows. No existing item or currency odds, lock, spawn, loot mode, spell 239916 or quest 46769 condition are changed. The change applies to every spawn of the two affected entries.

Source review confirmed the native gathering-node lookup reads Data1 and the exact reported log originates from the missing loot table. The user-owned worldserver was running when the report was received. The SQL is staged for the world updater at the next startup. No separate automated or startup test was run during this gameplay session. Both the first post-update client loot and the status of the diagnostic log remain open. Preserve the two live failures until loot is confirmed.
