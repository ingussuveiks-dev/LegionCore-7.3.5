# Legion 7.3.5 character boost type

An available distribution alone is not sufficient to show a character-select
boost button. The embedded product must have `Type = 1` and a nonzero
`CharacterServiceType` matching `CharacterServiceInfo.BoostType` (2 for level
100, 1 for level 90).

`CharacterServiceType` is the uint32 previously named `UnkInt4`, after `ItemId`
and before `UnkInt5` in the product packet. It is **not** the optional four-bit
`UnkBits` field. Setting only `UnkBits` produces a distribution grouped under
type 0: the client can recognize a paid purchase while displaying no boost
button.

Verified against the local 7.3.5.26972 x64 client executable:

- Product decoder at preferred VA `0x140503340` reads this uint32 into native
  product offset `0x118` (`0x140503408` through `0x140503417`).
- `GetUpgradeDistributions` helper at `0x1404CB470` requires status 1,
  a non-revoked distribution, an embedded product and product type 1. It
  groups the results by the uint32 at product offset `0x118`.
- The optional four-bit field is decoded into native offset `0x120` instead.
- `GlueXML/CharacterSelect.lua` indexes the returned amounts using the boost
  types returned by `GetCharacterServiceDisplayOrder`.

All three paths must populate `CharacterServiceType`: catalog products,
distribution updates and reconstructed pending distributions after login.
The distribution list must also flush its 11-bit count before writing objects.

Client acceptance check: reconnect an account with three unused level-100
credits, verify a level-100 boost button with count 3 at character select,
then assign one to an eligible character. The character should reach level
100 and the remaining balance should be 2 after another reconnect. Do not
purchase more credits to test visibility, or consume credits without the
account owner's choice of character and specialization.

The 7.3.5 character-select AssignUpgradeDistribution request can contain a
zero ProductID even with a valid distribution ID, target GUID and
specialization. Resolve an omitted product ID from the session's matching
pending distribution before validating delivery. Never substitute a default
product for an unknown distribution, or override an explicit mismatched ID.
Keep the account ownership, credit balance, level, specialization and faction
checks. A rejected assignment does not consume a credit or grant any rewards.

## Pandaren faction choice

The final assignment field is one uint32, not two independent uint16 values:
`(zeroBasedFaction << 24) | specializationId`. The wire faction is Horde=0,
Alliance=1; normalize it to the service's Horde=1, Alliance=2 only after
validating the upper byte. Validate the lower 24 bits before narrowing to
uint16, and retain the specialization/class check during assignment.

For specialization269, Horde sends0x0000010D and Alliance sends0x0100010D.
The former parser incorrectly read their choices as0 and256, respectively.
`BattlePayBoostChoice.h` now decodes and validates this field; the compile-time
regressions are in `tools/tests/BattlePayBoostChoiceTest.cpp`.
