param([switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/devourer-door-asan/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp" -Raw
 . "$PSScriptRoot/DevourerEscapeCertificate.ps1"
 $source=Restore-DevourerEscapeBoundary $source
 $old=(& git -C $repo show 'a8d3bb692:src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned door baseline missing'}
 if($Baseline){$source=$old}else{. "$PSScriptRoot/DevourerDoorCertificate.ps1";if(((Restore-DevourerDoorBoundary $source)-replace '\s','') -cne ($old-replace '\s','')){throw 'Unreviewed change outside final door'}}
 [IO.File]::WriteAllText("$out/DoorAI.inc",(Block $source 'class go_devourer_misc')+';')
 $scope=Get-Content "$repo/src/server/game/Entities/Object/QuestObjectScope.h" -Raw
 [IO.File]::WriteAllText("$out/QuestScope.inc",(Block $scope 'class QuestCancellationReceipt')+';'+(Block $scope "`nstruct QuestObjectScope")+';')
 $accessor=Get-Content "$repo/src/server/game/Globals/ObjectAccessor.h" -Raw
 [IO.File]::WriteAllText("$out/PlayerFind.inc",(Block $accessor 'static T* Find(ObjectGuid guid)'))
 [IO.File]::WriteAllText("$out/GenericLookup.inc",(Block $accessor 'template<class T> static T* GetObjectInWorld(ObjectGuid guid,'))
 [IO.File]::WriteAllText("$out/PlayerLookup.inc",(Block $accessor 'static Player* GetObjectInWorld(ObjectGuid guid, Player*')+(Block $accessor 'template<class T> static T* GetObjectInMap(')+'static Player* GetPlayer(WorldObject const&,ObjectGuid);')
 $native=Get-Content "$repo/src/server/game/Globals/ObjectAccessor.cpp" -Raw
 [IO.File]::WriteAllText("$out/PlayerLookupMethod.inc",(Block $native 'Player* ObjectAccessor::GetPlayer('))
 [IO.File]::WriteAllText("$out/GameObjectLookupMethod.inc",(Block $native 'GameObject* ObjectAccessor::GetGameObject('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $def=if($Baseline){'/DDOOR_BASELINE'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $def /I`"$out`" `"$PSScriptRoot/DevourerDoorTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b %errorlevel%`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd" > "$out/stdout.log" 2> "$out/stderr.log";$code=$LASTEXITCODE;Get-Content "$out/stdout.log";if($code -or (Get-Item "$out/stderr.log").Length){Get-Content "$out/stderr.log" -Tail 12;throw "Door ASAN exit=$code or stderr"}
}
