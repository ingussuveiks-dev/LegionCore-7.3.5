param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/nearest-player-asan/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $exports=@(
  @('src/server/game/Entities/Object/Object.cpp','NearestWorld.inc',@('Player* WorldObject::FindNearestPlayer(')),
  @('src/server/game/Grids/Notifiers/GridNotifiers.h','NearestCheck.inc',@('class NearestPlayerInObjectRangeCheck')),
  @('src/server/game/Grids/Notifiers/GridNotifiers.cpp','NearestMethods.inc',@('NearestPlayerInObjectRangeCheck::NearestPlayerInObjectRangeCheck(','bool NearestPlayerInObjectRangeCheck::operator()('))
 )
 foreach($row in $exports){$source=Get-Content "$repo/$($row[0])" -Raw;$old=(& git -C $repo show "2316b3592:$($row[0])") -join "`n";if($LASTEXITCODE){throw 'Pinned selector boundary missing'};$normalized=$source;$body=@(foreach($marker in $row[2]){$actual=Block $source $marker;$normalized=$normalized.Replace($actual,(Block $old $marker));$actual});if(($normalized-replace '\s','') -cne ($old-replace '\s','')){throw "Unexpected shared source change in $($row[0])"};[IO.File]::WriteAllText("$out/$($row[1])",($body -join "`n")+$(if($row[1] -eq 'NearestCheck.inc'){';'}else{''}))}
 $header=Get-Content "$repo/src/server/game/Grids/Notifiers/GridNotifiers.h" -Raw
 [IO.File]::WriteAllText("$out/NearestSearcher.inc",(Block $header 'struct PlayerLastSearcher')+';')
 $impl=Get-Content "$repo/src/server/game/Grids/Notifiers/GridNotifiersImpl.h" -Raw
 [IO.File]::WriteAllText("$out/NearestVisit.inc","template<class Check>`n"+(Block $impl 'void Trinity::PlayerLastSearcher<Check>::Visit('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/NearestPlayerTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b %errorlevel%`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd" > "$out/stdout.log" 2> "$out/stderr.log";$code=$LASTEXITCODE;Get-Content "$out/stdout.log";if($code -or (Get-Item "$out/stderr.log").Length){Get-Content "$out/stderr.log" -Tail 14;throw "Nearest-player ASAN exit=$code or stderr"}
}
