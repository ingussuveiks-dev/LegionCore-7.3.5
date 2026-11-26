-- Paid healing runs from the validated gossip option and the native heal spell.
DELETE FROM `smart_scripts`
WHERE `source_type`=0 AND `target_type`=7
    AND (`entryorguid`,`id`,`link`,`event_type`,`event_param1`,`event_param2`,`action_type`,`action_param1`) IN
        ((11069,0,1,62,9821,2,33,65214),(11069,1,0,61,0,0,85,133994),(63596,0,0,62,14991,1,85,133994));
