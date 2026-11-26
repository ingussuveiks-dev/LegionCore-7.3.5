# Sheilun pickup at the pool

The player reached the final stage and reported that a right-click on the visible staff produced no loot window. The character still had active quest 41003 at the pool without item 128937. Taran had already delivered the post-victory staff line. No packet capture identifies which interaction opcode the client used.

The private staff inherited UNIT_FLAG2_PREVENT_SELECT_NPC (0x04000000) from template 102479. Its source AI now clears this client targeting restriction when creating the victorious instance-owned staff. The private chest previously handled use and lock-spell delivery in the script, while a report-use packet dispatched to an unimplemented AI method. Both now dispatch through the chest AI to the same guarded instance Open. Repeated delivery for an already-open matching loot GUID reuses the existing window. The original owner, phase, range, LOS, victory, loot storage, claim spell and scene checks still govern acquisition.

Release compiled Sheylun.cpp and linked the private chest AI in the staging EXE (E8EDC3CEE7C742397D228A86EAE8966D3A0BC94C98590C4617E5617F196CCD8C). No separate runtime test or agent-owned startup was run during active client traversal. A fresh client pickup is pending.
