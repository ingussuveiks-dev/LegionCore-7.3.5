# Jormog's charge warning

The Legion DBM module includes a captured Jormog charge warning on `CHAT_MSG_MONSTER_EMOTE` and listens on that channel. The project already uses the original broadcast text `130585` and player target. Its text row had type `41` for a raid boss emote. Migration `2026_11_21_03_mage_tower_jormog_charge_emote.sql` gives that exact row native creature emote type `16`. [Original Legion DBM charge handler](https://github.com/DeadlyBossMods/DBM-Challenges/blob/cf9f66ea21818a6f503644599c5c4e1b28990a00/DBM-Challenges/LegionArtifacts/DPSFelTotem.lua).

Two disposable MariaDB applications and exact production readback confirmed the channel, original text and unchanged combat and owner-credit rows. The canonical Release ran and stopped without new warnings, errors, assertions or crashes. The evidence is under `build-extractors/bin/Release/fixed/2026-11-21-mage-tower-jormog-events/` and the actual client emote and DBM alert remain to be verified.
