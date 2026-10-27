-- Demons Among Us40607 is a PvE quest. Native faction2898 has the same
-- faction1786 and player/camp hostility as2780, without forced PvP.
-- Only the two revealed demon templates with native kill credit101105.
START TRANSACTION;
SET @demons_pve_safe :=
 (SELECT COUNT(*) FROM creature_template WHERE entry IN(101100,101104)
  AND faction IN(2780,2898) AND unit_flags=32768 AND unit_flags2=2048
  AND unit_flags3=0 AND flags_extra=0 AND AIName='' AND ScriptName='')=2
 AND (SELECT COUNT(*) FROM creature_template_wdb WHERE Entry IN(101100,101104) AND KillCredit1=101105 AND KillCredit2=0)=2
 AND (SELECT COUNT(*) FROM creature WHERE id IN(101100,101104))=34
 AND (SELECT COUNT(*) FROM creature WHERE id IN(101100,101104) AND map=1 AND PhaseId='7422' AND phaseMask=1)=34
 AND (SELECT COUNT(*) FROM quest_objectives WHERE QuestID=40607 AND Type=0 AND ObjectID=101105 AND Amount=12)=1;
SET @demons_pve_guard := IF(@demons_pve_safe,'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Demons Among Us native actors or quest differ; review PvE faction repair''');
PREPARE demons_pve_check FROM @demons_pve_guard;
EXECUTE demons_pve_check;
DEALLOCATE PREPARE demons_pve_check;
UPDATE creature_template SET faction=2898 WHERE @demons_pve_safe AND entry IN(101100,101104) AND faction=2780;
COMMIT;
