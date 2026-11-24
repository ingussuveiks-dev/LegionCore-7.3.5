# Rated Battleground matchmaking and maps

The rated queue found both groups without populating the pools read by map selection. Every rated map required more than zero players. The battleground factory also discarded the map passed by matchmaking and rerolled from a second list. The queue now supplies the matched rosters before selecting, and the factory creates the selected map. The selection preserves existing map veto and fallback behavior.

Build 26972 lists eight maps for rated queue 100. The rated pool uses those map IDs, activated templates and native ten-player ratings. Map 1803 is the additional Season 7 map. Rated Eye of the Storm uses queue 656, map 968 and its own two faction starts and six graveyards. Ordinary Eye of the Storm retains map 566 and its original starts and graves. [Blizzard's Season 7 announcement](https://worldofwarcraft.blizzard.com/en-us/news/21617654).

The focused regression exercised rated rosters, map eligibility, normal and arena pools, vetoes and fallback. The pinned pre-fix source failed the roster assertion as expected. The local DB2 catalog confirmed the queue and map IDs and the six graveyard records. MariaDB shadow application twice produced one rated template and preserved the original normal template.

The final link used the rebuilt Release game and scripts libraries. The map lists the linked queue, factory and Eye of the Storm objects. The installed binary SHA-256 is `86908680533EC3A57C1E9BACF7E7107747FEADB7BD5968465512987EA3D07491`. An owned canonical startup applied migration 16, loaded 43 battleground templates and exited normally after shutdown. No fresh DBErrors, server warnings or crashes were recorded. The build generator printed the archived revision fallback. Binary identity was checked against the staged hash.

A real 10v10 match, client invitation and loading, map mechanics, rating persistence and the RBG weekly reward remain client verification.
