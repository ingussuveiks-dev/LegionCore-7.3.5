-- 39860: approach Navarrogg; personal portal/Marakhan and owner-bound sacrifice.
UPDATE creature_template SET AIName='',ScriptName='npc_blood_rite_navarrogg',npcflag=npcflag & ~16777216 WHERE entry=95259;
UPDATE creature_template SET AIName='',ScriptName='npc_blood_rite_marakhan' WHERE entry=97418;
UPDATE creature_template SET AIName='NullCreatureAI',ScriptName='' WHERE entry=97675;
DELETE FROM npc_spellclick_spells WHERE npc_entry=95259 AND spell_id=192482;
DELETE FROM smart_scripts WHERE (source_type=0 AND entryorguid IN (95259,97418,97675)) OR (source_type=9 AND entryorguid IN (9741800,9767500));
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=95259 AND SourceId=0;
UPDATE creature_text SET Text='More souls! Come and accept your fate!' WHERE CreatureID=97418 AND GroupID=0 AND ID=0 AND BroadcastTextID=99494;
UPDATE creature_text SET Text='Me rip, you die!' WHERE CreatureID=97418 AND GroupID=1 AND ID=0 AND BroadcastTextID=82906;
