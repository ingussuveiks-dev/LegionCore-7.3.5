# Shimmer and the Displacement button

Bitabuta was in Frost (`64`) with Shimmer (`212653`) and a saved Displacement action (`212801`) in the Frost bar. The normal starter action table and boost bar have no Displacement entry. Matching 26972 `Talent` grants Shimmer to all mage specializations and replaces Blink `1953`. Both Displacement versions are listed for Arcane (`62`) in `SpecializationSpells`. `SpellLearnSpell` also describes replacement `212653 -> 212801` for base spell `195676`, with no specialization column. The core was applying that linked learn and override in Frost, which let the client show and save the unusable button.

`Player::addSpell` now scopes `212801` to Arcane. Registration of the matching `195676 -> 212801` override uses the same specialization check. The current specialization is set before login loads spells and before the new specialization's talents are loaded on a switch. Existing bar validation discards unknown spell actions; no character data or unrelated bar positions are edited directly. Shimmer's `1953` replacement and the existing Arcane learn, talent and cast paths remain available.

The matching DB2 data, saved action readback, previous client observation and learning order were inspected. Client login, talent toggles and Frost-to-Arcane switching on the repaired build remain unverified. No separate startup or gameplay test was run in the user-led flow.

The Release link map resolves `Player::addSpell` and `Player::AddOverrideSpell` to `Player.obj` from `game.lib` SHA-256 `78D5F6054978D0734806EE709A90352FDE4B7A91CFB39EF1F45611BAC5094269`. The staged executable SHA-256 is `0F17AC8D49E23B539F10E3DF667FA66727DBD2BFA6C2064118A5E8A2571717BD`. The user-owned worldserver still runs the current canonical build pending the next stop.
