UPDATE creature_template AS actor
JOIN creature_template_wdb AS native ON native.Entry=actor.entry
SET actor.flags_extra=actor.flags_extra | 128
WHERE actor.entry=94854 AND actor.flags_extra=0
  AND actor.AIName='' AND actor.ScriptName=''
  AND native.Displayid1=33225 AND native.Displayid2=64600
  AND native.Displayid3=0 AND native.Displayid4=0
  AND (SELECT COUNT(*) FROM creature WHERE id=94854)=1
  AND EXISTS (SELECT 1 FROM creature WHERE guid=361416 AND id=94854 AND map=1495
              AND modelid=0 AND phaseId='6363 5048 7000')
  AND EXISTS (SELECT 1 FROM gameobject WHERE guid=134175 AND id=248831 AND map=1495
              AND phaseId='6363 5048 7000');
