param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warrior-pickup-return/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$s,[string]$name){$a=$s.IndexOf($name);if($a -lt 0){throw "Missing $name"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 Copy-Item "$repo/src/server/scripts/Legion/ClassHalls/warrior_pickup_return.h" "$out/Controller.inc"
 Copy-Item "$repo/src/server/game/Entities/Object/QuestObjectScope.h" "$out/QuestObjectScope.inc"
 foreach($headerName in @('AreaTrigger.h','QuestData.h','ObjectGuid.h','DB2Stores.h')){[IO.File]::WriteAllText("$out/$headerName",'// Explicit controlled host endpoint.')}
 $flags=Get-Content "$repo/src/server/game/Quests/QuestDef.h" -Raw
 [IO.File]::WriteAllText("$out/QuestObjectiveFlags.inc",(Block $flags 'enum QuestObjectiveFlags')+';')
 $source=Get-Content "$repo/src/server/game/Entities/Conversation/Conversation.cpp" -Raw
 [IO.File]::WriteAllText("$out/ConversationUpdate.inc",(Block $source 'void Conversation::Update('))
 [IO.File]::WriteAllText("$out/ConversationAdmission.inc",(Block $source 'bool Conversation::CanNeverSee2('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/WarriorPickupReturnTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd" *> "$out/test.log";if($LASTEXITCODE){Get-Content "$out/test.log" -Tail 20;throw 'Warrior pickup return actual controller ASAN failed'}
 Get-Content "$out/test.log"|Where-Object {$_ -like 'PASS*'}
}
