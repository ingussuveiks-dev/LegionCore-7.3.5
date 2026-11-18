param([switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/dargal-progression-native/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/diagnostics" -RuntimeDirectories @($out) -Test {
 function Section([string]$source,[string]$start,[string]$end){$a=$source.IndexOf($start);$b=$source.IndexOf($end,[Math]::Max($a,0));if($a -lt 0 -or $b -le $a){throw "Missing actual source:$start"};$source.Substring($a,$b-$a)}
 $header="$repo/src/server/scripts/Legion/ClassHalls/keeping_the_peace.h";$map="$repo/src/server/game/Maps/Map.cpp";$player="$repo/src/server/game/Entities/Player/Player.cpp";$unit="$repo/src/server/game/Entities/Unit/Unit.cpp";$scope="$repo/src/server/game/Entities/Object/QuestObjectScope.h"
 $inputs=@($header,$map,$player,$unit,$scope,"$PSScriptRoot/DargalProgressionTest.cpp","$PSScriptRoot/Test-DargalProgression.ps1");$hashes=@{};foreach($p in $inputs){$hashes[$p]=(Get-FileHash -LiteralPath $p).Hash}
 Copy-Item -LiteralPath $header -Destination "$out/keeping_the_peace.h"
 $s=Get-Content $scope -Raw;[IO.File]::WriteAllText("$out/QuestScopeActual.inc",$s.Replace('#include "ObjectGuid.h"',''))
 $s=Get-Content $map -Raw;[IO.File]::WriteAllText("$out/MapBindActual.inc",(Section $s '    if (entry == 110635 && spellId == 228199' "    // These personal return actors must bypass93819"))
 $s=Get-Content $unit -Raw;[IO.File]::WriteAllText("$out/OwnerPlayerActual.inc",(Section $s 'Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const' 'Player* Unit::GetAffectingPlayer() const'))
 $s=Get-Content $player -Raw;[IO.File]::WriteAllText("$out/RecipientGuardActual.inc",(Section $s '        if (questid == 42771 && real_entry == 110635' '        // Lightning Rod tracks absorbed electricity'))
 $code=Get-Content $header -Raw;if($code -match '\b(KilledMonsterCredit|SetQuestObjectiveData|CompleteQuest|AddItem|TeleportTo)\s*\('){throw 'Service must not grant progression/items/travel'}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /MD /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/DargalProgressionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/compiler.pdb`" /link /INCREMENTAL:NO`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`nexit /b %errorlevel%`r`n")
 & "$out/run.cmd" *> "$out/native.log";if($LASTEXITCODE){Get-Content "$out/native.log" -Tail 15;throw 'Actual Dargal service native regression failed'}
 Get-Content "$out/native.log" -Tail 2
 foreach($p in $inputs){if((Get-FileHash -LiteralPath $p).Hash -ne $hashes[$p]){throw 'Native test source changed during run'}}
 $hashes|ConvertTo-Json|Set-Content "$out/inputs.json"
 if($VerifyLiveData){& "$PSScriptRoot/Test-SmartTerminalData.ps1" -ContextFile "$PSScriptRoot/DargalProgressionContext.json" -FixtureFile "$PSScriptRoot/DargalProgressionOriginal.json" -OutputDirectory "$out/live-data" -VerifyLiveData;if(!$?){throw 'Dargal actual updater/data differs'}}
}
