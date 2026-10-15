-- This core dispatches EVENTOBJECT_OFFTRIGGER=91. Event90 is not an EO exit.
UPDATE smart_scripts SET event_type=91
 WHERE source_type=13 AND id=1 AND event_type=90
 AND action_type=28 AND action_param1=219567 AND target_type=7
 AND entryorguid IN(124,134,145,147,148,158,165,167,177,181,198);
