# Artifact user-deletion protection, 2026-10-21

The user chose preventing artifact deletion, without replacement mail or quest
resets. HandleDestroyItemOpcode already rejected ITEM_FLAG_NO_USER_DESTROY
(0x20). The native 7.3.5 ItemSparse reader confirms all three Warlock artifacts
128941/128942/128943 carry that flag. The original client cannot ordinarily
destroy them through this opcode.

Across native ItemSparse, 45 rows have ArtifactID, of which 42 carry 0x20.
Unflagged rows are 127260 (Artifact 2, inventory type 0), 129738 (Artifact 3,
inventory type 21) and 134562 (Artifact 35, inventory type 13). These counts are
data records, not a claim of 45 obtainable specialization weapons. No native
records or metadata were invented or changed.

The server now also rejects a nonzero GetArtifactID in the user destruction
opcode. This preserves the same error and protects artifacts if an override
loses the flag. Both whole-item and partial-count deletion are blocked before
either destruction function. Ordinary items retain their prior handling.
Intentional script/quest item removal and GM operations are outside this
user-opcode change.

Test-ArtifactDeletion.ps1 compiles the complete actual production opcode and
executes flagged/unflagged artifact, ordinary item, whole/partial count and
equipped/missing/traded branches. The native DB2 catalog checks all artifact
rows and the three Warlock flags. The first fixture compile exposed a missing
default argument in its mock, which was corrected; production compilation
passed. A concurrent catalog link initially met a rebuilding game.lib; the
catalog was rerun successfully after that build completed.

The deployed combined EXE uses the rebuilt original game.lib and is installed
in build-extractors/bin/Release. Its SHA-256 is
`b42670b264b30ca862459280311aa8e7f40433d129d8607ea7ac105cc6317d57`.
Native regression and startup evidence are separate from the remaining client
check: attempting to drag an artifact out of a bag must reject deletion and
leave its GUID, AP, powers, relics and inventory slot intact. No actual player
weapon was destroyed or replaced during validation. Owned hidden worldserver
7676 reached ready and exited 0 after `server shutdown 0`; its new log segment,
DBErrors.log and stderr contained no errors. See the choice audit for exact
startup evidence and the validation-wrapper retry.

For the requested internet comparison, players report the same inability to
destroy artifacts in Blizzard's Customer Support forum:
https://us.forums.blizzard.com/en/wow/t/artifact-weapon-missing/408753 .
Those community observations are not treated as an official policy statement;
the local native records and opcode are the evidence for this server. Older
linked support articles redirected to generic help and supplied no usable
policy text during this audit.
