param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/smart-invoker-player"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_23_25_marksmanship_combat_conversations.sql" -Raw
$scope='((entryorguid=100394 AND id=0) OR (entryorguid=100749 AND id=4) OR (entryorguid=100836 AND id IN(6,8)))'
$reset="UPDATE smart_scripts SET target_type=7,target_param1=0 WHERE source_type=0 AND $scope;"
$sql=@"
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry IN(100394,100749,100836);
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
INSERT INTO creature_template SELECT * FROM saved_actor;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE action_type=206 OR (source_type=0 AND entryorguid IN(100394,100749,100836));
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
$reset
SELECT 'before',smart_scripts.* FROM smart_scripts WHERE NOT(source_type=0 AND $scope) ORDER BY entryorguid,source_type,id;
$migration
$migration
SELECT 'after',smart_scripts.* FROM smart_scripts WHERE NOT(source_type=0 AND $scope) ORDER BY entryorguid,source_type,id;
SELECT 'count',COUNT(*) FROM smart_scripts WHERE source_type=0 AND $scope AND target_type=130 AND target_param1=100;
SELECT 'row',entryorguid,id,event_type,event_flags,link,action_type,action_param1,target_type,target_param1 FROM smart_scripts WHERE source_type=0 AND $scope ORDER BY entryorguid,id;
$reset
UPDATE creature_template SET ScriptName='custom_combat' WHERE entry=100749;
$migration
SELECT 'custom-script',target_type FROM smart_scripts WHERE entryorguid=100749 AND source_type=0 AND id=4;
UPDATE creature_template SET ScriptName='' WHERE entry=100749;
$reset
UPDATE smart_scripts SET target_type=21,target_param1=50 WHERE entryorguid=100394 AND id=0;
$migration
SELECT 'custom-target',target_type FROM smart_scripts WHERE entryorguid=100394 AND id=0;
$reset
UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=100836 AND id=8;
$migration
SELECT 'custom-position',target_type FROM smart_scripts WHERE entryorguid=100836 AND id=8;
UPDATE smart_scripts SET action_param2=0 WHERE entryorguid=100836 AND id=8;
$reset
UPDATE smart_scripts SET action_param1=999 WHERE entryorguid=100394 AND id=0;
$migration
SELECT 'custom-conversation',target_type FROM smart_scripts WHERE entryorguid=100394 AND id=0;
SELECT 'actor',entry,id,actorId,creatureId,displayId,unk1,unk2,unk3,duration FROM conversation_actor WHERE entry IN(1123,1009,1555,1395) ORDER BY entry,id;
SELECT 'line',entry,id,idx,textId,unk1,unk2 FROM conversation_data WHERE entry IN(1123,1009,1555,1395) ORDER BY entry,id;
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Conversation target temporary SQL fixture failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("count`t4","custom-script`t7","custom-target`t21","custom-position`t7","custom-conversation`t7")){if($rows -notcontains $expected){throw "Conversation migration mismatch: $expected"}}
$before=@($rows | Where-Object {$_ -like "before`t*"}|ForEach-Object {$_.Substring(7)})
$after=@($rows | Where-Object {$_ -like "after`t*"}|ForEach-Object {$_.Substring(6)})
if(Compare-Object $before $after){throw 'Unrelated conversation/combat rows changed'}
[IO.File]::WriteAllLines("$out/data.tsv",[string[]]@($rows | Where-Object {$_ -match '^(actor|line)\t'}))
[IO.File]::WriteAllLines("$out/actions.tsv",[string[]]@($rows | Where-Object {$_ -like "row`t*"}))
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
[IO.File]::WriteAllText("$out/Owner.inc",(Section $source 'Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const' 'Player* Unit::GetAffectingPlayer() const'))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
[IO.File]::WriteAllText("$out/CreditPlayer.inc",(Section $source 'Player* GetScriptCreditPlayer(' 'Unit* GetScriptCreditCaster('))
[IO.File]::WriteAllText("$out/Resolver.inc",(Section $source 'namespace SmartInvokerPlayer' 'ObjectList* SmartScript::GetTargets('))
[IO.File]::WriteAllText("$out/Trigger.inc",(Section $source '    Unit* trigger = nullptr;' '    auto l = new ObjectList();'))
[IO.File]::WriteAllText("$out/PlayerTarget.inc",(Section $source '        case SMART_TARGET_ACTION_INVOKER_PLAYER:' '        case SMART_TARGET_POSITION:').Replace('l->','targets->').Replace('e.target.','event.target.'))
$targets=Section $source 'ObjectList* SmartScript::GetTargets(' 'ObjectList* SmartScript::GetWorldObjectsInDist('
$first=$targets.LastIndexOf('    if (GetBaseObject() && GetBaseObject()->HasQuestObjectScope())')
if($first -lt 0){throw 'Missing native final target-scope filter'}
$tail=$targets.Substring($first)
[IO.File]::WriteAllText("$out/ScopeTail.inc",([regex]::Replace($tail,'\bl\b','targets')))
$action=Section $source '        case SMART_ACTION_SUMMON_CONVERSATION:' '        case SMART_ACTION_SUMMON_ADD_PLR_PERSONNAL_VISIBILE:'
[IO.File]::WriteAllText("$out/ConversationAction.inc",$action.Replace('e.action.','event.action.').Replace('GetTargets(e,','GetTargets(event,'))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.cpp" -Raw
[IO.File]::WriteAllText("$out/Validation.inc",(Section $source '        case SMART_TARGET_PLAYER_DISTANCE:' '        case SMART_TARGET_PLAYER_RANGE:').Replace('e.target.','event.target.'))
$source=Get-Content "$repo/src/server/game/Globals/ConversationData.h" -Raw
[IO.File]::WriteAllText("$out/ConversationStructs.inc",(Section $source 'struct ConversationActor' 'typedef std::unordered_map'))
$source=Get-Content "$repo/src/server/game/Entities/Conversation/Conversation.cpp" -Raw
[IO.File]::WriteAllText("$out/ConversationData.inc",$source.Substring($source.IndexOf('bool Conversation::GenerateDynamicFieldData(')))
[IO.File]::WriteAllText("$out/Visibility.inc",(Section $source '    if (Player* player = caster->ToPlayer())' '    uint32 duration = 0;'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/SmartInvokerPlayerTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/data.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native player-invoker regression failed'}
if($VerifyReleaseBinary){$previousPython=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_smart_invoker_player_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical player-invoker PE verification failed'}}finally{$env:PYTHONPATH=$previousPython}}
'PASS: migration25 changes only4 supported combat conversation targets, preserves all other conversation/combat data and custom definitions'
