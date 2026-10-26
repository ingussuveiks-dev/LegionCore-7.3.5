param([switch]$AddressSanitizer)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/tuure-completion-tree"
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $old=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TuurePickupCatalog.cpp -ProbeName TuurePickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if($LASTEXITCODE){throw 'Tuure native completion catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$old}
 foreach($row in @("TREE`t6`t47678`t0`t0`t4`t0`t0","TREE`t6`t47679`t47678`t0`t0`t0`t29897","TREE`t6`t47680`t47678`t0`t0`t0`t29898","CRITERION`t6`t47679`t47678`t0`t29897`t92`t50679","CRITERION`t6`t47680`t47678`t0`t29898`t92`t50702")){if((Get-Content "$out/native.tsv") -notcontains $row){throw 'Native Tuure final tree changed'}}
 $enum=Get-Content "$repo/src/server/game/DataStores/DBCEnums.h" -Raw
 [IO.File]::WriteAllText("$out/CriteriaTypes.inc",(Block $enum 'enum CriteriaTypes')+';')
 [IO.File]::WriteAllText("$out/TreeOperators.inc",(Block $enum 'enum CriteriaTreeOperator')+';')
 $achievements=Get-Content "$repo/src/server/game/Achievements/AchievementMgr.cpp" -Raw
 [IO.File]::WriteAllText("$out/TreeCompletion.inc",(Block $achievements 'bool AchievementGlobalMgr::IsGroupCriteriaType(')+'template<typename Func>'+(Block $achievements 'void AchievementGlobalMgr::WalkCriteriaTree(')+"`ntemplate<class T>`n"+(Block $achievements 'bool AchievementMgr<T>::IsCompletedCriteria(')+"`ntemplate<class T>`n"+(Block $achievements 'bool AchievementMgr<T>::IsCompletedCriteriaTree(')+"`ntemplate<class T>`n"+(Block $achievements 'bool AchievementMgr<T>::IsCompletedScenarioTree('))
 $scenario=Get-Content "$repo/src/server/game/Scenarios/Scenario.cpp" -Raw
 [IO.File]::WriteAllText("$out/ScenarioCompletion.inc",((@('void Scenario::UpdateCurrentStep(','uint32 Scenario::GetScenarioCriteriaByStep(','bool Scenario::IsCompleted(','uint8 Scenario::GetStepCount(')|ForEach-Object {Block $scenario $_}) -join "`n"))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/TuureCompletionTreeTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`" `"$out/native.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Tuure completion regression failed'}
}
