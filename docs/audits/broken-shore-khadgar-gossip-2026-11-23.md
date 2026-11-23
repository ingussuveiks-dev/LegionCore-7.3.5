# Khadgar's Broken Shore assault dialogue

The active assault option in menu `20974`, slot `0`, had no BroadcastText reference. The server sent its stored option label directly, and the Legion client displayed the literal `$gready:ready;` token. Both alternatives are “ready”, so the migration replaces this exact label with “I'm ready; go on the attack.”

The migration changes only that label. Live data readback confirmed the corrected option, the separate `Here we go.` option, and the existing action menu values. The original option conditions and Khadgar's SmartAI spell and credit actions remain in place. The live world database records the migration as `RELEASED` with Windows updater text SHA-1 `8CDAF8F1D616D0871DBB5A1A6FADCA83D2321E18`.

The database update was made with the game servers stopped. The test character was already within interaction range of Khadgar on the correct map and had quest `46734` active. No character state was written. No test server or separate automated checks were run during the user's live quest check. The client still needs to confirm the label and the actual scenario transition.
