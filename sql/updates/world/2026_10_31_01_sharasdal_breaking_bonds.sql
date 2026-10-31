-- Code-coupled: keep outside live updater inputs until the matching Release
-- registers spell_sharasdal_breaking_bonds. Native208171/61 emits49816 itself.
-- Bind only its scoped original-Adelee/quest/AI-lifetime guard and presentation;
-- never add a rescue, guard, item, quest or scenario credit in SQL.
START TRANSACTION;
SELECT COUNT(*), COALESCE(SUM(AIName='' AND ScriptName='npc_adel_waterspeaker'
 AND npcflag=0 AND npcflag2=0 AND faction=1665 AND unit_flags=32768 AND unit_flags2=35653632),0)
 INTO @sharasdal_rescue_templates,@sharasdal_rescue_template_ok FROM creature_template WHERE entry=104853;
SELECT COUNT(*), COALESCE(SUM(npc_entry=104853 AND spell_id=208171 AND cast_flags=1 AND user_type=0 AND add_npc_flag=1),0)
 INTO @sharasdal_rescue_clicks,@sharasdal_rescue_click_ok FROM npc_spellclick_spells WHERE npc_entry=104853 OR spell_id=208171;
SELECT COUNT(*), COALESCE(SUM(ScenarioID=1066 AND Team=0 AND Class=7),0)
 INTO @sharasdal_rescue_scenarios,@sharasdal_rescue_scenario_ok FROM scenario_data WHERE MapID=1600;
SELECT COUNT(*), COALESCE(SUM(guid=362078 AND map=1600 AND PhaseId=6120 AND position_z<795),0),
 COALESCE(SUM(guid=362094 AND map=1600 AND PhaseId=6120 AND position_z>=795),0)
 INTO @sharasdal_rescue_spawns,@sharasdal_rescue_lower,@sharasdal_rescue_upper FROM creature WHERE id=104853;
SELECT COUNT(*) INTO @sharasdal_rescue_conditions FROM conditions WHERE
 (SourceTypeOrReferenceId=18 AND SourceGroup=104853) OR (SourceTypeOrReferenceId=17 AND SourceEntry=208171);
SELECT COALESCE(SUM(CASE WHEN spell_id=208171 AND ScriptName='spell_sharasdal_breaking_bonds' THEN 1 ELSE 0 END),0),
 COALESCE(SUM(CASE WHEN spell_id=208171 AND ScriptName='spell_sharasdal_breaking_bonds' THEN 0 ELSE 1 END),0)
 INTO @sharasdal_rescue_own_bindings,@sharasdal_rescue_other_bindings FROM spell_script_names WHERE ABS(spell_id)=208171;
SET @sharasdal_rescue_safe = @sharasdal_rescue_templates=1 AND @sharasdal_rescue_template_ok=1
 AND @sharasdal_rescue_clicks=1 AND @sharasdal_rescue_click_ok=1
 AND @sharasdal_rescue_scenarios=1 AND @sharasdal_rescue_scenario_ok=1
 AND @sharasdal_rescue_spawns=2 AND @sharasdal_rescue_lower=1 AND @sharasdal_rescue_upper=1
 AND @sharasdal_rescue_conditions=0 AND @sharasdal_rescue_other_bindings=0 AND @sharasdal_rescue_own_bindings IN(0,1);
SET @sharasdal_rescue_guard=IF(COALESCE(@sharasdal_rescue_safe,0),'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Sharasdal Breaking Bonds native context differs; review before binding''');
PREPARE sharasdal_rescue_check FROM @sharasdal_rescue_guard;
EXECUTE sharasdal_rescue_check;
DEALLOCATE PREPARE sharasdal_rescue_check;
INSERT INTO spell_script_names (spell_id,ScriptName)
 SELECT 208171,'spell_sharasdal_breaking_bonds'
 WHERE @sharasdal_rescue_safe AND @sharasdal_rescue_own_bindings=0;
COMMIT;
