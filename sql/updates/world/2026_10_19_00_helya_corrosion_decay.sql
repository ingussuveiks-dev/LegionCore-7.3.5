-- Orb of Corrosion's 228062 already invokes boss_helya_tov::SpellHit,
-- which casts 228063 at the orb from Helya. The old dummy route also cast
-- 228063 from the orb at Helya, duplicating damage/Decay with the wrong owner.
-- Keep the native periodic trigger, nearby-Helya condition and boss proxy.
DELETE FROM spell_dummy_trigger
WHERE spell_id = 228062 AND spell_trigger = 0 AND `option` = 11
  AND target = 0 AND caster = 0 AND targetaura = 0
  AND bp0 = 0 AND bp1 = 0 AND bp2 = 0
  AND effectmask = 1 AND handlemask = 8 AND aura = 0 AND `group` = 0;
