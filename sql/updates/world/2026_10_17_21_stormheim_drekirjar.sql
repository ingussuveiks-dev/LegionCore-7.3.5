-- Assault the Drekirjar (38374): Dragon-Blood Brew supplies native 1% credit
-- through183181 ->183167. Initial IN_USE blocks GameObject::Use before casting.
-- Preserve the consumable, respawn, brew buff/visual and unrelated flag bits.
UPDATE gameobject_template SET flags=flags & ~1
WHERE entry=241429 AND type=10 AND Data10=183181;
