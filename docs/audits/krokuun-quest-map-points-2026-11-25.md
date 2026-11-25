# Krokuun quest map points

Maiko had completed Threat Reduction (46841) and Prisoners No More (46840). The live world database had nine Legion 7.3.5 POI definitions for these quests and zero corresponding points. Both quests turn in to High Exarch Turalyon at the Seat of Annihilation, near 50.4, 53.8 on the Krokuun map. The local phase 1040 receiver is spawn 12844783 at world coordinates 1319.76, 1901.84. The phase 1060 receiver belongs to the later completion of A Strike at the Heart.

The migration fills the existing definitions for map 1669 and WorldMapArea 1135. It takes the slave polygon, kill locations and nearby giver points from the local TrinityCore 28153 POI archive. The archived index 32 coordinates are away from the local turn-in, so both completion points use the rounded phase 1040 receiver position. Indices with existing points and conflicting definitions are excluded. A Royal Audience (42985) already has its archived Tirisfal Glades and Undercity points on map 0.

A read-only MariaDB preview of the migration's selection returned 15 points for 46840 and five for 46841. The named turn-in NPC and phase were checked in the live data. No separate tests or owned startup ran during gameplay. The world updater and client map display remain pending.
