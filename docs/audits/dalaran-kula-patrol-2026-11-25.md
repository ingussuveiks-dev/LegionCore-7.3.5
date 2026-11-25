# Warp-Huntress Kula in Broken Isles Dalaran

The active NPC is entry `97512`, spawn `272369`, on map `1220`. Its spawn addon uses waypoint path `272369`. The older Dalaran also has an NPC of the same name, entry `32711`, on map `571`.

The original path matches the bundled 26972 world database. The waypoint generator draws straight splines between those points, including the last-to-first link. Three of those links intersect the city models. Extracted terrain and model collision put the worst sample 4.98 map units below the surface. The nearby spawn was 0.085 units low.

The migration raises the spawn to the first verified waypoint and rebuilds the walking loop around the supported streets and corners. It keeps the existing route binding and the main stops, removes the repeated stationary point, and returns along the verified outbound path. The added coordinates are reconstructed from the 26972 terrain, VMAP and navigation mesh.

The regression applies the migration twice in MariaDB temporary tables and checks the path binding, exact spawn alignment, stable replay, and isolation of the older Dalaran NPC. The original route reproduces all three major intersections. The adjusted route passes 1,578 floor samples and all 30 cyclic collision rays. The largest sampled floor intersection is 0.0121 units.

The checks produced no new crash or database error files and no new relevant server error. The migration is available to the canonical updater. The user chose to restart later. Live migration load and in-game observation remain pending.
