param([switch]$Baseline,[string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/fuzan-defense")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @("$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $inputs=@("$PSScriptRoot/FuZanDefenseTest.cpp","$PSScriptRoot/FuZanDefenseOriginal.json",$PSCommandPath,
  "$repo/tools/research/FuZanDefenseCatalog.cpp","$repo/src/server/game/AI/SmartScripts/SmartScript.cpp",
  "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h","$repo/src/server/game/Achievements/AchievementMgr.cpp",
  "$repo/src/server/game/DataStores/DBCEnums.h","$repo/src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/instance_temple_of_the_jade_serpent.cpp",
  "$repo/src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/fuzan_wave_entry.h",
  "$repo/src/server/game/Conditions/ConditionMgr.cpp","$repo/src/server/game/Conditions/ConditionMgr.h")
 $hashes=@{};foreach($path in $inputs){$hashes[$path]=(Get-FileHash -LiteralPath $path).Hash}
 function Section([string]$text,[string]$start,[string]$end){$i=$text.IndexOf($start);$j=$text.IndexOf($end,[Math]::Max(0,$i));if($i -lt 0 -or $j -le $i){throw "Missing production section $start"};$text.Substring($i,$j-$i)}
 $conditionSource=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
 [IO.File]::WriteAllText("$out/ConditionObjectGuard.inc",(Section $conditionSource '    // ASSERT(ConditionTarget < MAX_CONDITION_TARGETS);' '    bool condMeets = false;'))
 $conditionHeader=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.h" -Raw
 $targetCount=[regex]::Match($conditionHeader,'MAX_CONDITION_TARGETS\s*=\s*(\d+)').Groups[1].Value;if(!$targetCount){throw 'Native condition target bound missing'}
 [IO.File]::WriteAllText("$out/ConditionTargetConstants.inc","constexpr uint32 MAX_CONDITION_TARGETS=$targetCount;`n")
 $smart=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 $sections=[ordered]@{
  FindLinkedEvent=@('SmartScriptHolder SmartScript::FindLinkedEvent(','void SmartScript::OnUpdate(')
  LinkTail=@('    if (e.link && e.link != e.event_id)','void SmartScript::InstallTemplate(')
  EventDispatch=@('void SmartScript::ProcessEventsFor(','void SmartScript::ProcessAction(')
  EventPrefix=@('void SmartScript::ProcessEvent(','    switch (e.GetEventType())')
  ActionPrefix=@('void SmartScript::ProcessAction(','    switch (e.GetActionType())')
  TargetSelfVictim=@('        case SMART_TARGET_SELF:','        case SMART_TARGET_HOSTILE_SECOND_AGGRO:')
  DataSetEvent=@('        case SMART_EVENT_DATA_SET:','        case SMART_EVENT_PASSENGER_REMOVED:')
  LinkEvent=@('        case SMART_EVENT_LINK: //special handling','        //called from Update tick')
  NoParamEvents=@('        case SMART_EVENT_AGGRO:','        case SMART_EVENT_IS_BEHIND_TARGET:')
  TextOverEvent=@('        case SMART_EVENT_TEXT_OVER:','        case SMART_EVENT_DATA_SET:')
  WaypointEvent=@('        case SMART_EVENT_WAYPOINT_REACHED:','        case SMART_EVENT_SUMMON_DESPAWNED:')
  UpdateEvent=@('        case SMART_EVENT_UPDATE:','        case SMART_EVENT_UPDATE_OOC:')
  TalkAction=@('        case SMART_ACTION_TALK:','        case SMART_ACTION_SIMPLE_TALK:')
  SetDataAction=@('        case SMART_ACTION_SET_DATA:','        case SMART_ACTION_MOVE_FORWARD:')
  WaypointAction=@('        case SMART_ACTION_WP_START:','        case SMART_ACTION_WP_PAUSE:')
  TextTimer=@('    if (mUseTextTimer && me)','void SmartScript::FillScript(')
 }
 $all=''
 foreach($name in $sections.Keys){$v=Section $smart $sections[$name][0] $sections[$name][1];[IO.File]::WriteAllText("$out/$name.inc",$v);$all+=$v}
 $header=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
 $constants=@([regex]::Matches($all,'\bSMART_EVENT_[A-Z_]+\b')|ForEach-Object Value|Where-Object {$_ -notlike 'SMART_EVENT_FLAG_*'}|Sort-Object -Unique)
 $enum=@($constants|ForEach-Object {$m=[regex]::Match($header,"\b$_\s*=\s*(\d+)\s*,");if(!$m.Success){throw "Missing native event $_"};"$_=$($m.Groups[1].Value)"}) -join ','
 $extra=''
 foreach($flag in @([regex]::Matches($all,'\bSMART_EVENT_FLAG_[A-Z_]+\b')|ForEach-Object Value|Sort-Object -Unique)){$m=[regex]::Match($header,"\b$flag\s*=\s*([^,`r`n]+)");if(!$m.Success){throw "Missing native flag $flag"};$extra+="constexpr uint32 $flag=$($m.Groups[1].Value -replace '//.*','');`n"}
 foreach($name in @('SMART_ACTION_TALK','SMART_ACTION_SET_DATA','SMART_ACTION_WP_START','SMART_ESCORT_TARGETS')){$m=[regex]::Match($header,"\b$name\s*=\s*(0x[0-9A-Fa-f]+|[0-9]+)");if(!$m.Success){throw "Missing native constant $name"};$extra+="constexpr uint32 $name=$($m.Groups[1].Value);`n"}
 $dbc=Get-Content "$repo/src/server/game/DataStores/DBCEnums.h" -Raw
 foreach($m in [regex]::Matches($dbc,'\b(CRITERIA_TREE_OPER[A-Z_]+)\s*=\s*(\d+)')){$extra+="constexpr uint32 $($m.Groups[1].Value)=$($m.Groups[2].Value);`n"}
 [IO.File]::WriteAllText("$out/EventConstants.inc","enum SMART_EVENT {$enum}; constexpr uint32 SMART_TARGET_SELF=1,SMART_TARGET_VICTIM=2; $extra")
 $achievements=Get-Content "$repo/src/server/game/Achievements/AchievementMgr.cpp" -Raw
 $evaluator=Section $achievements 'bool AchievementMgr<T>::IsCompletedCriteriaTree(' 'template<class T>'
 [IO.File]::WriteAllText("$out/CriteriaEvaluator.inc","template<class T>`n$evaluator")
 $relative='src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/instance_temple_of_the_jade_serpent.cpp'
 $original=(& git -C $repo show "253c5ca40:$relative") -join "`n";if($LASTEXITCODE){throw 'Pinned original instance unavailable'}
 $current=Get-Content "$repo/$relative" -Raw
 $original=Section $original 'class instance_temple_of_the_jade_serpent' 'void AddSC_'
 $current=Section $current 'class instance_temple_of_the_jade_serpent' 'void AddSC_'
 # This extracted component tests the inherited native death dispatch, not the
 # wave-entry factory or its other virtual methods. The actual factory/vtable
 # is separately exercised by the native executable and selected PE checks.
 if($current.Contains('return new FuZanWaveEntry::Instance(map);')){
  $wave=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/fuzan_wave_entry.h" -Raw
  if($wave -notmatch 'class Instance : public InstanceScript, public FuZanVictoryRecipient' -or $wave -match '\b(OnUnitDeath|CreatureDies)\s*\('){throw 'Defense projection no longer inherits the native death hooks'}
  $current=$current.Replace('return new FuZanWaveEntry::Instance(map);','return new InstanceScript(map);')
  'SCOPE defense fixture projects inherited InstanceScript death hooks only; actual wave-entry factory requires native/PE checks'
 }
 [IO.File]::WriteAllText("$out/OriginalInstance.inc",$original.Replace('instance_temple_of_the_jade_serpent','OriginalTempleFactory'))
 [IO.File]::WriteAllText("$out/CurrentInstance.inc",$current)
 $fixture=Get-Content "$PSScriptRoot/FuZanDefenseOriginal.json" -Raw|ConvertFrom-Json
 # Compare the whole affected native graph to live records before using the
 # checked-in fixture. Every statement is read-only, with complete file I/O.
 $sql="SET SESSION TRANSACTION READ ONLY;`n"
 $filters=@{smart_scripts='entryorguid IN(109391,109438,109511,109933,110471,109821) AND source_type=0';conditions='SourceTypeOrReferenceId=22 AND SourceEntry IN(109391,109438,109511,109933,110471,109821)'}
 foreach($table in @('smart_scripts','conditions')){
  $pairs=($fixture.$table[0].PSObject.Properties.Name|ForEach-Object {"'$_',``$_``"}) -join ','
  $sql+="SELECT '$table',JSON_OBJECT($pairs) FROM $table WHERE $($filters[$table]);`n"
 }
 $sql+="SELECT 'INSTANCE',map,script FROM instance_template WHERE map=1625;`nSELECT 'EOF','fuzan-defense';`n"
 [IO.File]::WriteAllText("$out/live-data.sql",$sql)
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names') -InputFile "$out/live-data.sql" -OutputFile "$out/live-data.stdout" -ErrorFile "$out/live-data.stderr";if($code -or (Get-Item "$out/live-data.stderr").Length){throw 'Fu Zan readonly live data query failed'}}finally{$env:MYSQL_PWD=$previous}
 $live=Get-Content "$out/live-data.stdout"
 if($live[-1] -cne "EOF`tfuzan-defense" -or $live -cnotcontains "INSTANCE`t1625`tinstance_temple_of_the_jade_serpent"){throw 'Fu Zan instance/full EOF differs'}
 foreach($table in @('smart_scripts','conditions')){
  $actual=@($live|Where-Object {$_ -like "$table`t*"}|ForEach-Object {$_.Substring($table.Length+1)|ConvertFrom-Json})
  $expected=@($fixture.$table);if($actual.Count -ne $expected.Count){throw "Fu Zan native row count differs: $table"}
  foreach($row in $expected){
   $matches=@($actual|Where-Object {if($table -eq 'smart_scripts'){$_.entryorguid -eq $row.entryorguid -and $_.id -eq $row.id}else{$_.SourceEntry -eq $row.SourceEntry -and $_.SourceGroup -eq $row.SourceGroup -and $_.ConditionTypeOrReference -eq $row.ConditionTypeOrReference -and $_.ConditionValue1 -eq $row.ConditionValue1}})
   if($matches.Count -ne 1){throw "Fu Zan ambiguous/missing native row: $table"}
   foreach($property in $row.PSObject.Properties){
    if($table -eq 'conditions' -and $row.SourceEntry -eq 109391 -and $row.SourceGroup -eq 9 -and $property.Name -eq 'ConditionTarget'){
     if($matches[0].ConditionTarget -notin @(0,1)){throw 'Unknown native DATA condition target'};continue
    }
    if($matches[0].($property.Name) -cne $property.Value){throw "Fu Zan native field differs: $table/$($property.Name)"}
   }
  }
 }
 'PASS live native51 SmartAI/20 conditions; only known DATA8 target0/1 admitted, native UPDATE target0 unchanged'
 # Isolate the inherited-death regression from the separately reproduced
 # original DATA null-target defect. No live metadata is written here.
 foreach($row in $fixture.conditions){if($row.SourceEntry -eq 109391 -and $row.SourceGroup -eq 9){$row.ConditionTarget=1}}
 'SCOPE controlled death fixture projects DATA8 target1 and compiles actual missing-object guard; full native NULL/phase proof is separate'
 foreach($table in @('smart_scripts','conditions')){
  $lines=@($fixture.$table|ForEach-Object {($_.PSObject.Properties.Value|ForEach-Object {[Convert]::ToString($_,[Globalization.CultureInfo]::InvariantCulture)}) -join "`t"})
  [IO.File]::WriteAllLines("$out/$table.tsv",[string[]]$lines)
 }
 $old=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile FuZanDefenseCatalog.cpp -ProbeName FuZanDefenseCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$old}
 $native=Get-Content "$out/native.tsv"
 foreach($line in @("TREE`t49567`t49171`t9`t100`t0","TREE`t49568`t49567`t0`t5`t31014","TREE`t49620`t49567`t0`t2`t31026","CRITERION`t49568`t31014`t92`t52970`t0`t0","CRITERION`t49620`t31026`t92`t53046`t0`t0","CRITERION`t49177`t30719`t92`t52981`t0`t0","CRITERION`t49351`t30874`t92`t52590`t0`t0")){if($native -notcontains $line){throw "Native1137 defense changed: $line"}}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $mode=if($Baseline){'baseline'}else{'repaired'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/FuZanDefenseTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/smart_scripts.tsv`" `"$out/conditions.tsv`" $mode`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Fu Zan defense instance/native dialogue regression failed'}
 foreach($path in $inputs){if((Get-FileHash -LiteralPath $path).Hash -ne $hashes[$path]){throw 'Fu Zan test input changed during execution'}}
 [IO.File]::WriteAllText("$out/input-manifest.json",($hashes|ConvertTo-Json -Depth 3))
}
