START TRANSACTION;

UPDATE areatrigger_template
SET VisualID=230829
WHERE entry=0 AND spellId=0 AND customEntry=11869 AND VisualID=57404
AND Radius=5 AND RadiusTarget=5 AND Height=0 AND HeightTarget=0 AND isMoving=0
AND (SELECT COUNT(*) FROM areatrigger_actions WHERE customEntry=11869)=2
AND EXISTS (SELECT 1 FROM areatrigger_actions WHERE entry=0 AND customEntry=11869 AND id=0 AND moment=1 AND actionType=0 AND targetFlags=8 AND spellId=215997)
AND EXISTS (SELECT 1 FROM areatrigger_actions WHERE entry=0 AND customEntry=11869 AND id=1 AND moment=42 AND actionType=1 AND targetFlags=8 AND spellId=215997);

COMMIT;
