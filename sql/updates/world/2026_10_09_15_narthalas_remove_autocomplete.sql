-- These three lesson start scripts bypassed their actual objectives. Loading
-- command 7 also silently restores the event flag removed by migrations 13/14.
UPDATE quest_template SET StartScript=0 WHERE ID IN (37729,42370,42371) AND StartScript=ID;
DELETE FROM quest_start_scripts WHERE id IN (37729,42370,42371) AND command=7 AND datalong=id;
DELETE FROM quest_start_scripts WHERE id=42371 AND command=17 AND datalong IN (137423,137422,137426);
UPDATE quest_template_addon SET SpecialFlags=SpecialFlags & ~2 WHERE ID IN (37729,42370,42371);
