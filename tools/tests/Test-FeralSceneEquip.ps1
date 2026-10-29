param([switch]$VerifyLiveData,[switch]$AddressSanitizer)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/feral-scene-equip"
New-Item -ItemType Directory -Force $out|Out-Null
$path="$repo/sql/updates/world/2026_10_29_03_feral_scene_equip.sql"
$migration=Get-Content $path -Raw
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
   # by @feral_safe; the expected SIGNAL is a counterfactual, not a pass.
   if($errors -notmatch 'Feral native scene/equip conditions differ; review before applying' -or
      @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected refusal diagnostics: $errorFile"}
  }elseif($code -ne 0 -or $errors){throw "Feral SQL failed (exit $code): $errorFile"}
  return ,$rows
 }finally{$env:MYSQL_PWD=$previous}
}

$setup=''
foreach($table in @('spell_scene','spell_scene_event','conditions')){
 $filter=@{spell_scene='MiscValue IN(1282,1235,1166)';spell_scene_event='MiscValue IN(1282,1235,1166)';conditions='SourceTypeOrReferenceId=17 AND SourceEntry IN(214715,207562,199960)'}[$table]
 $setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $filter;CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"
}
$reset="DELETE FROM spell_scene_event WHERE MiscValue=1282;DELETE FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=214715;"
function Snapshot([string]$label){
 $sql=''
 foreach($table in @('spell_scene','spell_scene_event','conditions')){
  $order=@{spell_scene='MiscValue';spell_scene_event='MiscValue,Event';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference'}[$table]
  $sql+="SELECT '$label-$table',t.* FROM $table t ORDER BY $order;`n"
 }
 return $sql
}
function Signature($rows,[string]$label){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)}))}
$rows=Query ($setup+$migration+$reset+(Snapshot 'before')+$migration+(Snapshot 'first')+$migration+(Snapshot 'second')+@"
SELECT 'event',MiscValue,Event,trigerSpell,MonsterCredit FROM spell_scene_event WHERE MiscValue=1282;
SELECT 'rule',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=214715 ORDER BY ElseGroup,ConditionTypeOrReference;
"@)
[IO.File]::WriteAllLines("$out/temp.tsv",[string[]]$rows)
foreach($table in @('spell_scene','spell_scene_event','conditions')){
 if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Scene migration not idempotent: $table"}
 if($table -eq 'spell_scene' -and (Signature $rows "before-$table") -cne (Signature $rows "first-$table")){throw 'Native scene changed'}
 if($table -ne 'spell_scene'){
  $prefix=if($table -eq 'spell_scene_event'){'^1282\t'}else{'^17\t[^\t]+\t214715\t'}
  $before=@((Signature $rows "before-$table") -split "`n"|Where-Object {$_ -notmatch $prefix})
  $after=@((Signature $rows "first-$table") -split "`n"|Where-Object {$_ -notmatch $prefix})
  if(($before -join "`n") -cne ($after -join "`n")){throw "Other native scene/equip rows changed: $table"}
 }
}
$variants=[ordered]@{
 'package'='UPDATE spell_scene SET SceneScriptPackageID=1605 WHERE MiscValue=1282;'
 'flags'='UPDATE spell_scene SET PlaybackFlags=0 WHERE MiscValue=1282;'
 'duration'='UPDATE spell_scene SET CustomDuration=1000 WHERE MiscValue=1282;'
 'script'="UPDATE spell_scene SET ScriptName='custom' WHERE MiscValue=1282;"
 'event'="INSERT INTO spell_scene_event VALUES(1282,'custom',1,0,'custom');"
 'condition'="INSERT INTO conditions VALUES(17,0,214715,0,0,4,0,1,0,0,0,0,'','custom');"
 'applied-spell'="UPDATE spell_scene_event SET trigerSpell=199960 WHERE MiscValue=1282;"
 'applied-credit'="UPDATE spell_scene_event SET MonsterCredit=107750 WHERE MiscValue=1282;"
 'applied-condition'='UPDATE conditions SET ConditionValue1=1 WHERE SourceTypeOrReferenceId=17 AND SourceEntry=214715 AND ConditionTypeOrReference=40;'
 'applied-missing'='DELETE FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=214715 AND ElseGroup=1 AND ConditionTypeOrReference=40;'
 'applied-same-count'="DELETE FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=214715 AND ElseGroup=1 AND ConditionTypeOrReference=40;INSERT INTO conditions VALUES(17,0,214715,0,1,4,0,1,0,0,0,0,'','custom');"
}
foreach($name in $variants.Keys){
 $base=$setup+$migration+$(if($name -notlike 'applied-*'){$reset}else{''})
 $result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
 [IO.File]::WriteAllLines("$out/refusal-$name.tsv",[string[]]$result)
 foreach($table in @('spell_scene','spell_scene_event','conditions')){
  if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom scene $name overwritten: $table"}
 }
}
$events=@($rows|Where-Object {$_ -like "event`t*"}|ForEach-Object {$_.Substring(6)})
$rules=@($rows|Where-Object {$_ -like "rule`t*"}|ForEach-Object {$_.Substring(5)})
if($events.Count -ne 1 -or $events[0] -cne "1282`tcomplete`t214715`t0" -or $rules.Count -ne 33){throw 'Native Feral event/rules differ'}
[IO.File]::WriteAllLines("$out/events.tsv",[string[]]$events)
[IO.File]::WriteAllLines("$out/rules.tsv",[string[]]$rules)
& "$PSScriptRoot/Test-FeralArtifactLoot.ps1" -AddressSanitizer:$AddressSanitizer
$fixture="$repo/build-extractors/runtime-test/feral-artifact-loot"
foreach($name in @('Conditions','ElseGroups','SmartEventConditions','RootDispatch','LootCondition','SlotRecheck','GetLootId')){Copy-Item "$fixture/$name.inc" "$out/$name.inc"}
function Section([string]$s,[string]$a,[string]$b){$first=$s.IndexOf($a);$last=$s.IndexOf($b,$first);if($first -lt 0 -or $last -le $first){throw "Missing native $a"};$s.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
[IO.File]::AppendAllText("$out/Conditions.inc",(Section $source '        case CONDITION_SCENE_SEEN:' '        case CONDITION_SCENE_TRIGER_EVENT:'))
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
[IO.File]::WriteAllText("$out/Lifecycle.inc",(Section $source 'void Player::SendSpellScene(' 'void Player::HandleArenaDeserter()'))
$source=Get-Content "$repo/src/server/game/Handlers/SceneHandler.cpp" -Raw
[IO.File]::WriteAllText("$out/Handlers.inc",$source.Substring($source.IndexOf('void WorldSession::HandleScenePlaybackCanceled(')))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
foreach($mode in @('conditions','scene')){
 $test=if($mode -eq 'conditions'){'TalonclawPickupAdmissionTest.cpp'}else{'FeralSceneEquipTest.cpp'}
 $fixtureArguments=if($mode -eq 'conditions'){"`"$out/rules.tsv`" feral-cast 250386"}else{"`"$out/events.tsv`""}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/$test`" /Fe`"$out/$mode.exe`" /Fo`"$out/$mode.obj`" /Fd`"$out/$mode.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/$mode.exe`" $fixtureArguments`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw "Native Feral scene $mode failed"}
}
if($VerifyLiveData){
 $hash=(Get-FileHash $path -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_29_03_feral_scene_equip.sql';SELECT 'event',MiscValue,Event,trigerSpell,MonsterCredit FROM spell_scene_event WHERE MiscValue=1282;SELECT 'rules',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=214715;"
 foreach($line in @("release`t$hash`tRELEASED","event`t1282`tcomplete`t214715`t0","rules`t33")){if($live -notcontains $line){throw "Live Feral scene differs: $line"}}
}
'PASS: native Feral completion mapping, 33 real cast rules, 11 full original/applied custom refusals, preserved other scenes, native instance lifecycle and final-stage ownership/scene gates. Actual client presentation and whole acquisition remain separate.'
