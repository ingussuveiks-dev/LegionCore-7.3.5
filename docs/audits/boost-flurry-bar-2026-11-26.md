# Flurry on the Frost Mage tutorial bar

On entering A Flurry of Frost the player saw four populated slots and an empty fifth slot. A read-only check found Flurry (44614) learned, active and enabled. Scenario 1132 step 2972 uses tree 55460, whose child 55461 tracks two casts of hidden spell 229824. The Flurry criterion for a later tree uses 44614 directly.

The boost overlay now translates 229824 to 44614 before its active-spell and duplicate checks. Normal step updates and rebuilding the bar while the lesson is current use the same path. The fifth slot is available when the preceding four tutorial spells are present. Saved character buttons and the scenario's credit criteria are untouched.

Release rebuilt Player.cpp into the original game library. The staged link used that library and its map resolves Player::SendActionButtons to game:Player.obj. Staged EXE SHA-256 is C82988F01E54274ABB2382212851F0B63F0526727BDA778E711B68545A91630D. No separate automated test or server startup was run during this active gameplay report. Client slot display and actual subsequent lesson progress await in-game confirmation.

Promoted after the user stopped worldserver. The canonical EXE matches the staged SHA-256 above, and its map matches 4860C6C29897E143EB2C02FC499182EF6EC154F1DDC1C4FAFC48C51DB70A83ED. The preceding pair was retained under .codex/boost-flurry-bar-20261126/rollback for the pending retest. Removed the two verified duplicates from its stage subdirectory (270403690 bytes). Before the client retest, the canonical Crashes directory and DBErrors.log were empty.

## Client retest and empowered Flurry credit

The client's next screenshot shows Flurry in the fifth slot and the lesson still at 0/2. The damage numbers alone do not identify whether that cast had Brain Freeze. The normal Frostbolt proc path handles passive 190447 and its aura 190446. Flurry already snapshots that aura before the cast and uses it for its missile behavior. Neither its script nor the world spell trigger and linked-spell tables issued the self-targeted dummy spell 229824 that the native CAST_SPELL criterion requires.

The primary Flurry dummy effect now issues that acknowledgement on a successful empowered hit in tree 55460. Only the player's direct cast on the selected training dummy sharing its ship in scenario 1132 qualifies. The three damage missiles and pet spells do not handle this criterion. Ordinary Flurry casts and other lessons retain their existing rules.

The staged Release link used the original scripts library and the map resolves HandleEmpoweredHit to spell_mage.obj. The staged EXE SHA-256 is 1F7780C6B8770627261C3F8258121846B6DDB0434C930C33807B850D2E78BA59. No agent-owned server startup or separate regression was run during the user's active traversal. Both counter increments await a client retest.
