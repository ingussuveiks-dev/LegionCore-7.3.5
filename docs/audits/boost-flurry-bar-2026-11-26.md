# Flurry on the Frost Mage tutorial bar

On entering A Flurry of Frost the player saw four populated slots and an empty fifth slot. A read-only check found Flurry (44614) learned, active and enabled. Scenario 1132 step 2972 uses tree 55460, whose child 55461 tracks two casts of hidden spell 229824. The Flurry criterion for a later tree uses 44614 directly.

The boost overlay now translates 229824 to 44614 before its active-spell and duplicate checks. Normal step updates and rebuilding the bar while the lesson is current use the same path. The fifth slot is available when the preceding four tutorial spells are present. Saved character buttons and the scenario's credit criteria are untouched.

Release rebuilt Player.cpp into the original game library. The staged link used that library and its map resolves Player::SendActionButtons to game:Player.obj. Staged EXE SHA-256 is C82988F01E54274ABB2382212851F0B63F0526727BDA778E711B68545A91630D. No separate automated test or server startup was run during this active gameplay report. Client slot display and actual subsequent lesson progress await in-game confirmation.
