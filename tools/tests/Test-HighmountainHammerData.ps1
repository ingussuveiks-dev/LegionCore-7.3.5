$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$sql=@"
SELECT 'objective',COUNT(*) FROM quest_objectives WHERE QuestID=42454;
SELECT 'native-objective',COUNT(*) FROM quest_objectives WHERE ID=284276 AND QuestID=42454 AND Type=0 AND StorageIndex=3 AND ObjectID=107587 AND Amount=1 AND Flags=0 AND Flags2=0;
SELECT 'item',COUNT(*) FROM quest_template WHERE ID=42454 AND StartItem=137649 AND ItemDrop1=137649 AND ItemDropQuantity1=1 AND FlagsEx=0;
SELECT 'chain',COUNT(*) FROM quest_template_addon WHERE ID=42454 AND PrevQuestID=39781 AND ProvidedItemCount=1;
SELECT 'starter',COUNT(*) FROM gameobject_queststarter WHERE id=250548 AND quest=42454;
SELECT 'ender',COUNT(*) FROM gameobject_questender WHERE id=246463 AND quest=42454;
SELECT 'pads',COUNT(*) FROM gameobject_template WHERE entry IN(244534,244560) AND type=5 AND ScriptName='go_eye_portrait_teleporter';
SELECT 'upper',COUNT(*) FROM gameobject WHERE guid=110106 AND id=244534 AND map=1220 AND zoneId=7502 AND ABS(position_x+844.597)<0.01 AND ABS(position_y-4467.76)<0.01 AND ABS(position_z-736.042)<0.01;
SELECT 'lower',COUNT(*) FROM gameobject WHERE guid=110181 AND id=244560 AND map=1220 AND zoneId=7502 AND ABS(position_x+779.958)<0.01 AND ABS(position_y-4415.29)<0.01 AND ABS(position_z-602.629)<0.01;
SELECT 'arrival',COUNT(*) FROM spell_target_position WHERE id=192293 AND target_map=1220 AND ABS(target_position_x+784.91)<0.01 AND ABS(target_position_y-4419.19)<0.01 AND ABS(target_position_z-602.48)<0.01;
SELECT 'return',COUNT(*) FROM spell_target_position WHERE id=192295 AND target_map=1220 AND ABS(target_position_x+846.86)<0.01 AND ABS(target_position_y-4469.6)<0.01 AND ABS(target_position_z-736.04)<0.01;
SELECT 'duplicate-triggers-disabled',COUNT(*) FROM eventobject_template WHERE entry IN(4,5) AND radius=0 AND SpellID=0;
SELECT 'questgiver',COUNT(*) FROM gameobject g JOIN gameobject_template t ON t.entry=g.id WHERE g.guid=25178662 AND g.id=246463 AND g.map=1220 AND g.zoneId=7502 AND g.areaId=8045 AND g.PhaseId='4590' AND t.type=2 AND t.displayId=29572;
SELECT 'placed-hammer',COUNT(*) FROM creature c JOIN creature_template_wdb t ON t.Entry=c.id JOIN creature_template_addon a ON a.entry=c.id WHERE c.guid=11800937 AND c.id=121494 AND c.map=1220 AND c.zoneId=7502 AND c.areaId=8045 AND c.PhaseId='4589' AND t.DisplayId1=76105 AND a.auras='154441';
SELECT 'phase-definitions',COUNT(*) FROM phase_definitions WHERE zoneId=7502 AND flags=16 AND ((entry=218 AND phaseId=4590) OR (entry=219 AND phaseId=4589));
SELECT 'conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7502 AND SourceEntry IN(218,219);
SELECT 'native-conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7502 AND ElseGroup=0 AND ConditionTypeOrReference=8 AND ConditionValue1=42454 AND ((SourceEntry=218 AND NegativeCondition=1) OR (SourceEntry=219 AND NegativeCondition=0));
SELECT CONCAT('phase-',stage,'-',SourceEntry),IF(NegativeCondition,NOT(stage=3),stage=3) FROM (SELECT 0 stage UNION ALL SELECT 1 UNION ALL SELECT 2 UNION ALL SELECT 3) stages CROSS JOIN conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7502 AND SourceEntry IN(218,219) ORDER BY stage,SourceEntry;
"@
$expected=@("objective`t1","native-objective`t1","item`t1","chain`t1","starter`t1","ender`t1","pads`t2","upper`t1","lower`t1","arrival`t1","return`t1","duplicate-triggers-disabled`t2","questgiver`t1","placed-hammer`t1","phase-definitions`t2","conditions`t2","native-conditions`t2")
foreach($stage in 0..3){$expected+="phase-$stage-218`t$([int]($stage -ne 3))";$expected+="phase-$stage-219`t$([int]($stage -eq 3))"}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$c[3]
 $actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names
 if($LASTEXITCODE){throw 'Hammer SQL test failed'}
} finally {$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Hammer SQL mismatch: $($actual -join '|')"}
'PASS: live native Hammer objective/items/handoff, both pads/destinations, gallery quest giver and placed visual, and all four quest-status phase states. Read-only checks.'
