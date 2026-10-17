-- Drive Back the Tideskorn (39998): allow the original banner/circle use.
-- GameObject::Create copies template flags; Use rejects GO_FLAG_IN_USE (1)
-- before the native 193190/193191 spell can award its 2%/5% credit.
-- Keep unrelated flag bits, spells, visuals, consumable/respawn settings and
-- the banner's separate Grey Shoals Supplies interaction script.
UPDATE gameobject_template SET flags=flags & ~1
WHERE entry IN(244703,244704,244729,244730,244731,244733) AND type=10;
