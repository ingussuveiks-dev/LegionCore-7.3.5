param([switch]$Baseline,[switch]$AdmissionBaseline,[switch]$AddressSanitizer,[switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop'
if($Baseline -and $AdmissionBaseline){throw 'Select one counterfactual'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/tuure-defeat"
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/$(if($Baseline){'baseline'}elseif($AdmissionBaseline){'admission-baseline'}else{'current'})" -RuntimeDirectories @($out,$repo) -Test {
 $path='src/server/scripts/Scenario/Artifacts/Priest/Tuure/Tuure.cpp';$current=Get-Content "$repo/$path" -Raw;$source=$current
 if($Baseline){$source=(& git -C $repo show "ce34ea6c:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Tuure defeat baseline unavailable'}}
 if($AdmissionBaseline){$source=(& git -C $repo show "8a8608e1:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned native-criteria admission baseline missing'}}
 [IO.File]::WriteAllText("$out/Defeat.inc",(Block $(if($AdmissionBaseline){$source}else{$current}) 'namespace TuureDefeat')+(Block $source 'struct npc_lady_kalindriaAI :')+';')
 $nativeCriteria=Get-Content "$repo/src/server/game/Achievements/AchievementMgr.cpp" -Raw
 $gate=$nativeCriteria.IndexOf('if (cachePtr->player->isGameMaster() || !cachePtr->player->CanContact())');$end=$nativeCriteria.IndexOf('return;',$gate)
 if($gate -lt 0 -or $end -lt $gate){throw 'Native GM/contact delivery gate missing'}
 [IO.File]::WriteAllText("$out/NativeCreditGate.inc",$nativeCriteria.Substring($gate,$end+7-$gate))
 $u=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
 [IO.File]::WriteAllText("$out/OwnerGuid.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'ObjectGuid GetCharmerOrOwnerGUID() const'))
 [IO.File]::WriteAllText("$out/OwnerPlayer.inc",(Block $u 'Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'))
 $a=$u.IndexOf('    damage /= victim->GetHealthMultiplierForTarget(this);');$b=$u.IndexOf('    // A player charmed by an NPC',$a)
 if($a -lt 0 -or $b -le $a){throw 'Native post-scaling dispatch missing'}
 [IO.File]::WriteAllText("$out/ScaledHook.inc",$u.Substring($a,$b-$a))
 $eh=Get-Content "$repo/src/server/game/AI/EventMap.h" -Raw;$es=Get-Content "$repo/src/server/game/AI/EventMap.cpp" -Raw
 [IO.File]::WriteAllText("$out/EventMap.inc",(Block $eh 'enum c_events')+';'+(Block $eh 'class TC_GAME_API EventMap').Replace('TC_GAME_API ','')+';'+((@('void EventMap::Reset(','void EventMap::ScheduleEvent(uint32 eventId, uint32 time','uint32 EventMap::ExecuteEvent(','void EventMap::CancelEvent(')|ForEach-Object {Block $es $_}) -join "`n"))
 $fh=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$fs=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
 [IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $fh 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$fs.Substring($fs.IndexOf('FunctionProcessor::FunctionProcessor()')))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $def=if($Baseline){'/DBASELINE'}elseif($AdmissionBaseline){'/DADMISSION_BASELINE'}else{''};if($AddressSanitizer){$def+=' /fsanitize=address /Zi'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $def /I`"$out`" `"$PSScriptRoot/TuureDefeatTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Tuure defeat native regression failed'}
 if($VerifyNativeCatalog){
  $savedCatalog=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TuurePickupCatalog.cpp -ProbeName TuureCriteriaCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native Tuure criteria catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$savedCatalog}
  $rows=Get-Content "$out/native.tsv"
  foreach($id in @(29834,29833,30023,30011,29863,29864,29879,29897,29898)){if($rows -notcontains ("CRITERIA_META`t$id"+"`t0"*9)){throw "Native Tuure criterion$id has additional gates; review before admitting dead owner"}}
  $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
  $query=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'criteria-data',COUNT(*) FROM achievement_criteria_data WHERE criteria_id IN(29834,29833,30023,30011,29863,29864,29879,29897,29898);
SELECT 'criteria-disables',COUNT(*) FROM disables WHERE (sourceType=4 AND entry IN(29834,29833,30023,30011,29863,29864,29879,29897,29898)) OR (sourceType=10 AND entry IN(47498,47519,47882,47883,47561,47909,47656,47679,47680));
'@
  $savedSqlPwd=$env:MYSQL_PWD
  try{$env:MYSQL_PWD=$db[3];$live=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Native Tuure criteria override query failed'}}finally{$env:MYSQL_PWD=$savedSqlPwd}
  if(Compare-Object @("criteria-data`t0","criteria-disables`t0") $live){throw 'Tuure criteria overrides/disables changed; preserve and review them'}
  $live|Set-Content -Encoding utf8 "$out/live.tsv"
  'PASS native nine criteria have no additional modifier/start/fail/timer/flag/world-state gates or live criteria overrides/disables; native delivery retains GM/contact filter'
 }
}
