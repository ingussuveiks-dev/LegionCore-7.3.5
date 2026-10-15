-- Portal to Tol Barad (43153): display 8834 / FileDataID 165828 has
-- Stand (0, 1667ms, opening flash), Hold (158, 3334ms), Decay (159).
-- With no state animation override the client repeats Stand's opening flash.
-- Native kit 2133 plays Stand once and then loops only Hold (segment 1).
-- Keep the existing goober spell, scenario selector, model and interaction.
UPDATE gameobject_template SET SpellStateAnimKitID=2133 WHERE entry=251699;
