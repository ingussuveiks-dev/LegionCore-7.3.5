-- A stationary scouting-map model must not fall through the class-hall floor.
-- Keep the native model, pose, interaction flags and AdventureMap menu.
UPDATE creature_template_movement SET Ground=0, Swim=0, Flight=1, Rooted=1
 WHERE CreatureId=101979;
