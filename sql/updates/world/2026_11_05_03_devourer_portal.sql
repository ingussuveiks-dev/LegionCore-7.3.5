-- Promote the matching executable before exposing this migration to the updater.
-- Native26972 Val'zuun: acceptance, incantation, completed gateway.
DELETE FROM creature_text WHERE CreatureID=105488 AND GroupID IN(0,2,3) AND ID=0;
INSERT INTO creature_text(CreatureID,GroupID,ID,Text,Type,Language,Probability,Emote,Duration,Sound,BroadcastTextID,MinTimer,MaxTimer,SpellID,comment) VALUES
(105488,0,0,"Ered'achor, shudas'urzul... kurai.",12,0,100,0,0,0,108250,0,0,0,'Valzuun41924 native incantation'),
(105488,2,0,'It is done. The way to Shadowgore Citadel is open.',12,0,100,0,0,0,108257,0,0,0,'Valzuun41924 native gateway ready'),
(105488,3,0,'Thank you for seeing reason.',12,0,100,0,0,0,108249,0,0,0,'Valzuun41924 native acceptance');
UPDATE gameobject_template SET ScriptName='go_devourer_misc' WHERE entry=248901 AND ScriptName IN('','go_devourer_misc');
