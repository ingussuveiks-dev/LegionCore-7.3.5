-- These map1533 environmental actors retain their native visuals/selection
-- flags. Disable gravity, including at ground level; Flight2 alone allows
-- flight but does not disable falling. Their script supplies actual splines.
INSERT INTO creature_template_movement(CreatureId,Ground,Swim,Flight,Rooted,Random) VALUES
(100835,0,0,1,0,0),(100932,0,0,1,0,0)
ON DUPLICATE KEY UPDATE Ground=0,Flight=1,Rooted=0;
UPDATE creature_template SET AIName='',ScriptName='npc_dark_riders_ambient'
WHERE entry IN(100835,100932) AND ScriptName IN('','npc_dark_riders_ambient');
