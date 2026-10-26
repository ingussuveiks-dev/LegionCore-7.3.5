-- Promote to updates/world only with the verified Dreadblades pickup executable.
-- The old activation callback is incompatible with this restored native loot.
START TRANSACTION;
UPDATE gameobject_template SET Data1=254087
WHERE entry=254087 AND type=3 AND displayId=9806 AND Data0=1691 AND Data1=0
AND AIName='' AND ScriptName='go_scenario_dreadblades'
AND (SELECT COUNT(*) FROM gameobject_loot_template WHERE Entry=254087)=1
AND EXISTS (SELECT 1 FROM gameobject_loot_template WHERE Entry=254087 AND Item=128872 AND Reference=0 AND Chance=100 AND QuestRequired=0 AND MinCount=1 AND MaxCount=1);
COMMIT;
