param([switch]$AddressSanitizer,[switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/broken-shore-class-routes"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){if($s[$b] -eq '{'){++$depth};if($s[$b] -eq '}'){--$depth};++$b};$s.Substring($a,$b-$a)}
function Admission([string]$s){$a=$s.IndexOf('        if (IsScenario() || sObjectMgr->HasScenarioInMap(GetId()))');$b=$s.IndexOf('        // check for existing instance binds',$a);if($a -lt 0 -or $b -le $a){throw 'Native map admission missing'};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($out,$repo,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 [IO.File]::WriteAllText("$out/Selector.inc",(Block (Get-Content "$repo/src/server/game/Globals/ObjectMgr.cpp" -Raw) 'ScenarioData const* ObjectMgr::GetScenarioOnMap('))
 [IO.File]::WriteAllText("$out/ResumeDungeon.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/PlayerScenario.cpp" -Raw) 'lfg::LFGDungeonData const* ResumeDungeon('))
 [IO.File]::WriteAllText("$out/Admission.inc",(Admission (Get-Content "$repo/src/server/game/Maps/Map.cpp" -Raw)))
 $old=(& git -C $repo show 'c12cf6071:src/server/game/Maps/Map.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned baseline absent'}
 [IO.File]::WriteAllText("$out/Baseline.inc",(Admission $old))
 $s=Get-Content "$repo/src/server/game/Scenarios/Scenario.cpp" -Raw;$a=$s.IndexOf('    if (player->GetScenarioId())');$b=$s.IndexOf('    _scenarioEntry =',$a)
 [IO.File]::WriteAllText("$out/Constructor.inc",$s.Substring($a,$b-$a))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncd /d `"$out`"`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/BrokenShoreClassRouteTest.cpp`" /Fe:test.exe /Fo:test.obj /Fd:test.pdb`nif errorlevel 1 exit /b 1`ntest.exe`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Broken Shore native route regression failed'}
 if($VerifyNativeCatalog){$oldConfig=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile WarriorIntroCatalog.cpp -ProbeName WarriorIntroCatalog -OutputFile "$out/catalog.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$oldConfig};$catalog=Get-Content "$out/catalog.tsv" -Raw;foreach($expected in @('LFG\t906\t775\t12','LFG\t1166\t961\t12','LFG\t1176\t0\t12','STEP\t933\t4\t45776','CRITERIA\t45777\t92\t46881','MAP_DIFFICULTY\t1500\t12')){if($catalog -notmatch $expected){throw "Native26972 route differs: $expected"}};'PASS native26972 scenario/LFG/step/criteria metadata with hotfix overlays'}
}
