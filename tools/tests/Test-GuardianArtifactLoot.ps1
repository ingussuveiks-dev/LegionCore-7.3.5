param([switch]$VerifyLiveData,[switch]$AddressSanitizer)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/guardian-artifact-loot"
New-Item -ItemType Directory -Force $out|Out-Null
$migrationPath="$repo/sql/updates/world/2026_10_29_01_guardian_artifact_loot.sql"
$migration=Get-Content $migrationPath -Raw
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$script:queryIndex=0
function Query([string]$sql,[switch]$Refusal){
 $previous=$env:MYSQL_PWD
 $errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
 try{
  $env:MYSQL_PWD=$connection[3]
  $ErrorActionPreference='Continue'
  $rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names --force 2>$errorFile)
  $code=$LASTEXITCODE
  [IO.File]::WriteAllText("$errorFile.exit.txt",[string]$code)
  $ErrorActionPreference='Stop'
  $errors=Get-Content $errorFile -Raw
  if($Refusal){
   # --force permits the post-refusal snapshot. Every mutation is also guarded
   # by @guardian_safe; the expected SIGNAL is a counterfactual, not a pass.
   if($errors -notmatch 'Guardian native chest/loot/actions/conditions differ; review before applying' -or
      @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected refusal diagnostics: $errorFile"}
  }elseif($code -ne 0 -or $errors){throw "Guardian SQL failed (exit $code): $errorFile"}
  return ,$rows
 }finally{$env:MYSQL_PWD=$previous}
}
$scope='(SourceTypeOrReferenceId=4 AND SourceGroup=248853) OR (SourceTypeOrReferenceId=22 AND SourceEntry=248853 AND SourceId=1)'
$setup=''
$filters=@{gameobject_template='entry IN(248853,248499,250386)';gameobject_loot_template='Entry IN(248853,248499,250386)';smart_scripts='entryorguid IN(248853,248499,250386) AND source_type=1';conditions='(SourceTypeOrReferenceId=4 AND SourceGroup IN(248853,248499,250386)) OR (SourceTypeOrReferenceId=22 AND SourceEntry IN(248853,248499,250386) AND SourceId=1)'}
foreach($table in @('gameobject_template','gameobject_loot_template','smart_scripts','conditions')){
 $setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"
}
# Validate original or exactly-applied live rows with the migration's full
# fingerprints before reconstructing the old state in session-local tables.
$baseline=@"
$migration
UPDATE gameobject_template SET Data1=0 WHERE entry=248853;
DELETE FROM conditions WHERE $scope;
INSERT INTO conditions VALUES(22,3,248853,1,0,2,0,128821,1,0,0,0,'','Smart Only If Player Has Artifact');
"@
function SnapshotSql([string]$label){
 $sql=''
 foreach($table in @('gameobject_template','gameobject_loot_template','smart_scripts','conditions')){
  $sql+="SELECT '$label-$table',t.* FROM $table t ORDER BY "+@{gameobject_template='entry';gameobject_loot_template='Entry,Item';smart_scripts='entryorguid,source_type,id';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference'}[$table]+";`n"
 }
 return $sql
}
$sql=$setup+$baseline+(SnapshotSql 'before')+$migration+(SnapshotSql 'first')+$migration+(SnapshotSql 'second')+@"
SELECT 'loot',g.entry,g.type,g.Data1,l.Item FROM gameobject_template g JOIN gameobject_loot_template l ON l.Entry=g.Data1 WHERE g.entry=248853;
SELECT 'rule',SourceTypeOrReferenceId,SourceGroup,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $scope ORDER BY SourceTypeOrReferenceId,SourceGroup,ElseGroup,ConditionTypeOrReference;
"@
$rows=Query $sql
[IO.File]::WriteAllLines("$out/temp-rows.tsv",[string[]]$rows)
function Signature($values,[string]$label){[string]::Join("`n",[string[]]@($values|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)}))}
foreach($table in @('gameobject_template','gameobject_loot_template','smart_scripts','conditions')){
 if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Guardian migration is not idempotent: $table"}
 if($table -in @('gameobject_loot_template','smart_scripts')){
  if((Signature $rows "before-$table") -cne (Signature $rows "first-$table")){throw "Guardian native $table changed"}
 }
}
if($rows -notcontains "loot`t248853`t3`t248853`t128821"){throw 'Guardian native loot link differs'}
foreach($table in @('gameobject_template','conditions')){
 $before=@(Signature $rows "before-$table") -split "`n"
 $after=@(Signature $rows "first-$table") -split "`n"
 if($table -eq 'gameobject_template'){
  # Compare the entire native template, normalizing only Data1 (column 17).
  $after=@($after|ForEach-Object {$v=$_.Split("`t");if($v[0] -eq '248853'){$v[17]='0'};$v -join "`t"})
 }else{
  $before=@($before|Where-Object {$_ -notmatch '^22\t[^\t]+\t248853\t1\t' -and $_ -notmatch '^4\t248853\t'})
  $after=@($after|Where-Object {$_ -notmatch '^22\t[^\t]+\t248853\t1\t' -and $_ -notmatch '^4\t248853\t'})
 }
 if(($before -join "`n") -cne ($after -join "`n")){throw "Unrelated/native fields changed: $table"}
}
$variants=[ordered]@{
 'template-flags'='UPDATE gameobject_template SET flags=flags+1 WHERE entry=248853;'
 'template-display'='UPDATE gameobject_template SET displayId=9807 WHERE entry=248853;'
 'template-script'="UPDATE gameobject_template SET ScriptName='custom' WHERE entry=248853;"
 'template-link'='UPDATE gameobject_template SET Data1=1 WHERE entry=248853;'
 'loot-chance'='UPDATE gameobject_loot_template SET Chance=50 WHERE Entry=248853;'
 'loot-reference'='UPDATE gameobject_loot_template SET Reference=1 WHERE Entry=248853;'
 'loot-currency'='UPDATE gameobject_loot_template SET Currency=1 WHERE Entry=248853;'
 'loot-item'='UPDATE gameobject_loot_template SET Item=128306 WHERE Entry=248853;'
 'smart-link'='UPDATE smart_scripts SET link=0 WHERE entryorguid=248853 AND id=0;'
 'smart-handoff'='UPDATE smart_scripts SET target_param1=101390 WHERE entryorguid=248853 AND id=3;'
 'smart-extra'="INSERT INTO smart_scripts(entryorguid,source_type,id,comment) VALUES(248853,1,99,'custom');"
 'condition-custom'='UPDATE conditions SET ConditionValue3=1 WHERE SourceEntry=248853 AND SourceTypeOrReferenceId=22;'
 'condition-extra'="INSERT INTO conditions VALUES(4,248853,128821,0,0,4,0,1,0,0,0,0,'','custom');"
}
foreach($name in $variants.Keys){
 $result=Query ($setup+$baseline+$variants[$name]+(SnapshotSql 'before')+$migration+(SnapshotSql 'after')) -Refusal
 [IO.File]::WriteAllLines("$out/refusal-$name.tsv",[string[]]$result)
 foreach($table in @('gameobject_template','gameobject_loot_template','smart_scripts','conditions')){
  if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom $name was overwritten: $table"}
 }
}
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
function Section([string]$text,[string]$start,[string]$end){$a=$text.IndexOf($start);$b=$text.IndexOf($end,[Math]::Max(0,$a));if($a -lt 0 -or $b -le $a){throw "Missing native $start"};$text.Substring($a,$b-$a)}
$cases=(Section $source '        case CONDITION_ITEM:' '        case CONDITION_ITEM_EQUIPPED:')+(Section $source '        case CONDITION_CLASS:' '        case CONDITION_RACE:')+(Section $source '        case CONDITION_QUESTREWARDED:' '        case CONDITION_QUEST_NONE:')+(Section $source '        case CONDITION_MAPID:' '        case CONDITION_AREAID:')+(Section $source '        case CONDITION_SCENARION_STEP:' '        case CONDITION_REALM_ACHIEVEMENT:')+(Section $source '        case CONDITION_ALIVE:' '        case CONDITION_HP_VAL:')
[IO.File]::WriteAllText("$out/Conditions.inc",$cases)
[IO.File]::WriteAllText("$out/ElseGroups.inc",(Section $source 'bool ConditionMgr::IsObjectMeetToConditionList(' 'bool ConditionMgr::IsObjectMeetToConditions(WorldObject* object,'))
[IO.File]::WriteAllText("$out/SmartEventConditions.inc",(Section $source 'bool ConditionMgr::IsObjectMeetingSmartEventConditions(' 'bool Condition::isLoaded() const'))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
[IO.File]::WriteAllText("$out/RootDispatch.inc",(Section $source 'void SmartScript::ProcessEventsFor(' 'void SmartScript::ProcessAction('))
$source=Get-Content "$repo/src/server/game/Loot/LootMgr.cpp" -Raw
[IO.File]::WriteAllText("$out/LootCondition.inc",(Section $source 'bool LootItem::AllowedForPlayer(Player const* player) const' 'void LootItem::AddAllowedLooter('))
$tail=Section $source '    if (item && !AllowedForPlayer(player,' 'uint32 Loot::GetMaxSlotInLootFor('
[IO.File]::WriteAllText("$out/SlotRecheck.inc",$tail.Substring(0,$tail.LastIndexOf('}')))
$source=Get-Content "$repo/src/server/game/Entities/GameObject/GameObjectData.cpp" -Raw
[IO.File]::WriteAllText("$out/GetLootId.inc",(Section $source 'uint32 GameObjectTemplate::GetLootId() const' 'uint32 GameObjectTemplate::GetGossipMenuId() const'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($mode in @('loot','root','gossip')){
 $kind=if($mode -eq 'loot'){4}else{22};$group=if($mode -eq 'loot'){248853}elseif($mode -eq 'root'){1}else{3}
 $rules=@($rows|Where-Object {$_ -like "rule`t$kind`t$group`t*"}|ForEach-Object {($_.Split("`t")|Select-Object -Skip 3) -join "`t"})
 if($rules.Count -ne $(if($mode -eq 'loot'){14}else{28})){throw "Wrong native rule count for $mode"}
 [IO.File]::WriteAllLines("$out/$mode.tsv",[string[]]$rules)
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/TalonclawPickupAdmissionTest.cpp`" /Fe`"$out/$mode.exe`" /Fo`"$out/$mode.obj`" /Fd`"$out/$mode.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/$mode.exe`" `"$out/$mode.tsv`" guardian-$mode 248853`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw "Native Guardian $mode failed"}
}
if($VerifyLiveData){
 $hash=(Get-FileHash $migrationPath -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_29_01_guardian_artifact_loot.sql';SELECT 'link',Data1 FROM gameobject_template WHERE entry=248853;SELECT 'rules',COUNT(*) FROM conditions WHERE $scope;"
 foreach($line in @("release`t$hash`tRELEASED","link`t248853","rules`t70")){if($live -notcontains $line){throw "Guardian live readback differs: $line"}}
}
'PASS: Guardian actual-schema repeat migration, 13 complete custom refusals, native loot selector/conditions/ElseGroup/cached-slot/root/gossip matrices. Original native presentation, credit, NPC handoff and other Druid routes preserved; client traversal remains pending.'
