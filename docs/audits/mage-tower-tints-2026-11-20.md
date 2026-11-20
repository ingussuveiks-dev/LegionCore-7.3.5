# Legion Mage Tower tint prerequisites

On build 26972, the 108 alternate colors have separate achievement
requirements and reference the base look's native modifier tree. The
Legion modifier evaluator lacks the challenge quest-state operators that
the original trees reference. The generated hotfixes now require the
same completed challenge quest as the base look through the existing
PlayerCondition quest bitmap check. They retain each native modifier
tree and achievement. The hotfixes notify the client and are appended
after existing generated notices.

The native catalog regression confirmed the 36 bases, 108 distinct
tint conditions, seven nonzero quest bitmap flags, original tree assets,
and all 108 compatible empty previous-quest fields. The staged link
used the CMake Release libraries. The linker map places the DB2 load
implementation in `game:DB2Stores.obj`. The installed executable hash
matched the tested staged image. Both the staged and installed Release
image started with the isolated database and shut down cleanly without
new crashes, warnings, assertions or DBErrors. Local evidence is under
`build-extractors/bin/Release/fixed/2026-11-20-mage-tower-repair/`.

Client verification still includes the forge before victory, after the
base look, and after each independent tint achievement. The startup
and native catalog checks establish the prerequisites and server load,
with the client interaction still open.
