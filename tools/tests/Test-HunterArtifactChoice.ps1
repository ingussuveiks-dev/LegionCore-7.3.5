param([string]$MigrationPath="$PSScriptRoot/../../sql/updates/world/2026_10_26_12_hunter_additional_artifact_choice.sql",[switch]$AddressSanitizer)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hunter-artifact-choice"
New-Item -ItemType Directory -Force $out | Out-Null
function Block([string]$text,[string]$marker){$a=$text.IndexOf($marker);if($a -lt 0){throw "Missing $marker"};$end=$text.IndexOf('{',$a)+1;$depth=1;while($depth -and $end -lt $text.Length){if($text[$end] -eq '{'){$depth++};if($text[$end] -eq '}'){$depth--};$end++};if($depth){throw "Unbalanced $marker"};$text.Substring($a,$end-$a)}
function Save($name,$value){[IO.File]::WriteAllText("$out/$name.inc",$value)}
$player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
Save 'Display' (Block $player 'void Player::SendDisplayPlayerChoice(')
$gossip=Get-Content "$repo/src/server/game/Entities/Creature/GossipDef.h" -Raw
$a=$gossip.IndexOf('enum class GossipOptionNpc');$b=$gossip.IndexOf('//POI icons',$a);$enums=$gossip.Substring($a,$b-$a)
$unit=Get-Content "$repo/src/server/game/Entities/Unit/UnitDefines.h" -Raw
$a=$unit.IndexOf('enum NPCFlags :');$b=$unit.IndexOf('enum MovementFlags',$a);Save 'Enums' ($enums+$unit.Substring($a,$b-$a))
$gossip=Get-Content "$repo/src/server/game/Entities/Creature/GossipDef.cpp" -Raw
$a=$gossip.IndexOf('uint64 GossipMenu::GetRequiredNpcFlagForOption');$b=$gossip.IndexOf('PlayerMenu::PlayerMenu',$a);Save 'RequiredFlags' $gossip.Substring($a,$b-$a)
$filter=[regex]::Match($player,'            if \(!\(GossipMenu::GetRequiredNpcFlagForOption\(optionNpc\) & npcflags\)\)\s*continue;')
if(!$filter.Success){throw 'Production gossip flag filter changed'};Save 'Admission' $filter.Value

Save 'Handle' (Block (Get-Content "$repo/src/server/game/Handlers/MiscHandler.cpp" -Raw) 'void WorldSession::HandleChoiceResponse(')
foreach($p in @(@('Map','void Player::UpdateAvailableQuestLines('),@('Take','bool Player::CanTakeQuest('),@('Conditions','bool Player::SatisfyQuestConditions('),@('QuestStatus','bool Player::SatisfyQuestStatus('),@('Level','bool Player::SatisfyQuestLevel('),@('Class','bool Player::SatisfyQuestClass('),@('Race','bool Player::SatisfyQuestRace('))){Save $p[0] (Block $player $p[1])}
Save 'Status' ((Block (Get-Content "$repo/src/server/game/Quests/QuestDef.h" -Raw) 'enum QuestStatus')+';')
$cond=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
$cases=@('CONDITION_CLASS','CONDITION_QUESTREWARDED','CONDITION_QUEST_COMPLETE','CONDITION_QUEST_NONE','CONDITION_LEVEL','CONDITION_QUESTTAKEN') | ForEach-Object {$a=$cond.IndexOf("        case $_"+':');$b=$cond.IndexOf('        case ',$a+13);$cond.Substring($a,$b-$a)}
Save 'Cases' ($cases -join "`n")
$a=$cond.IndexOf('    if (NegativeCondition)');$b=$cond.IndexOf("`n}",$a);Save 'Tail' $cond.Substring($a,$b-$a)
Save 'List' (Block $cond 'bool ConditionMgr::IsObjectMeetToConditionList(')
$migration=Get-Content -LiteralPath $MigrationPath -Raw
$sql=''
foreach($table in @('conditions','gossip_menu_option','smart_scripts','creature_template')){
 $filter=switch($table){'conditions' {"(SourceTypeOrReferenceId=15 AND SourceGroup IN(19115,20227)) OR (SourceTypeOrReferenceId=32 AND SourceEntry=240) OR (SourceTypeOrReferenceId=33 AND SourceEntry IN(504,505,506))"} 'gossip_menu_option' {'MenuID IN(19115,20227)'} default {'entry'+$(if($table -eq 'smart_scripts'){'orguid'})+' IN(102478,107317,107973)'}}
 $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table WHERE $filter; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
}
# Recreate the pinned missing-interaction baseline in TEMP tables even after
# the live updater has installed this repair. All unrelated fixture rows remain.
$sql+="UPDATE saved_creature_template SET npcflag=2 WHERE entry=107317; DELETE FROM saved_conditions WHERE Comment LIKE 'Hunter artifact choice - %'; DELETE FROM saved_gossip_menu_option WHERE MenuID=20227 AND OptionID=0 AND OptionBroadcastTextID=103645; DELETE FROM saved_smart_scripts WHERE comment LIKE 'Hunter artifact choice - %';"
$sql+="INSERT INTO saved_conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1,Comment) SELECT 15,19115,0,8,40618,'Show gossip option if Player complete Quest 40618' WHERE NOT EXISTS(SELECT 1 FROM saved_conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=19115 AND SourceEntry=0);"
foreach($table in @('conditions','gossip_menu_option','smart_scripts','creature_template')){$sql+="DELETE FROM $table;INSERT INTO $table SELECT * FROM saved_$table;`n"}
$sql+=@"
$migration
$migration
SELECT 'SAFE',@hunter_offer_safe;
SELECT 'CONDITIONS',COUNT(*) FROM conditions WHERE Comment LIKE 'Hunter artifact choice - %';
SELECT 'FLAGS',COUNT(*) FROM creature_template WHERE entry IN(107317,107973) AND (npcflag&3)=3;
SELECT 'MENU',COUNT(*) FROM gossip_menu_option WHERE MenuID=20227 AND OptionID=0 AND OptionBroadcastTextID=103645;
SELECT 'SAI',COUNT(*) FROM smart_scripts WHERE comment LIKE 'Hunter artifact choice - %';
SELECT 'PRESERVED',COUNT(*) FROM smart_scripts WHERE entryorguid=107973 AND id=0 AND event_type=20 AND event_param1=42659 AND action_type=85 AND action_param1=216477;
SELECT 'DALARAN',COUNT(*) FROM gossip_menu_option WHERE MenuID=19115;
SELECT 'ACTOR',entry,npcflag,npcflag2 FROM creature_template WHERE entry IN(107317,107973);
SELECT 'C',SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE Comment LIKE 'Hunter artifact choice - %' ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup;
"@
# A custom row must refuse the whole migration, preserving flags/menu/events too.
foreach($conflict in @("INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1,Comment) VALUES(33,0,505,27,110,NULL);","INSERT INTO gossip_menu_option(MenuID,OptionID,OptionText,OptionBroadcastTextID) VALUES(20227,0,'custom',0);","INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,comment) VALUES(107973,0,1,19,1,'custom');")){
 foreach($table in @('conditions','gossip_menu_option','smart_scripts','creature_template')){$sql+="DELETE FROM $table;INSERT INTO $table SELECT * FROM saved_$table;`n"}
 $sql+="$conflict`n$migration`nSELECT 'REFUSED',@hunter_offer_safe;SELECT 'UNTOUCHED',COUNT(*) FROM creature_template WHERE entry=107317 AND npcflag=2;SELECT 'NOEVENTS',COUNT(*) FROM smart_scripts WHERE comment LIKE 'Hunter artifact choice - %';`n"
}
$db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$prior=$env:MYSQL_PWD
try {$env:MYSQL_PWD=$db[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Hunter choice TEMP SQL fixture failed'}}finally{$env:MYSQL_PWD=$prior}
foreach($expected in @("SAFE`t1","CONDITIONS`t249","FLAGS`t2","MENU`t1","SAI`t3","PRESERVED`t1","DALARAN`t3")){if($rows -notcontains $expected){throw "Missing migration result $expected"}}
foreach($expected in @("REFUSED`t0","UNTOUCHED`t1","NOEVENTS`t0")){if(@($rows|Where-Object {$_ -eq $expected}).Count -ne 3){throw "Custom row refusal failed $expected"}}
[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]@($rows|Where-Object {$_ -like "C`t*"}|ForEach-Object {$_.Substring(2)}))
[IO.File]::WriteAllLines("$out/actors.tsv",[string[]]@($rows|Where-Object {$_ -like "ACTOR`t*"}|ForEach-Object {$_.Substring(6)}))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 $asan /Fd`"$out/test.pdb`" /I`"$out`" `"$PSScriptRoot/HunterArtifactChoiceTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /link /INCREMENTAL:NO`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/conditions.tsv`" `"$out/actors.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native Hunter artifact choice regression failed'}
'PASS TEMP migration repeated twice, both actor flags/menu/events, original campaign reward and Dalaran options preserved; three custom-slot refusal variants. No live rows or characters changed.'
