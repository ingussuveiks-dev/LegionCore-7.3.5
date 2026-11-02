param([switch]$Baseline,[switch]$AddressSanitizer)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$mode=if($Baseline){'baseline'}else{'fixed'}
$out="$repo/build-extractors/runtime-test/smart-set-data-$mode/run-$([DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fffffff'))"
New-Item -ItemType Directory -Force $out | Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/diagnostics" -RuntimeDirectories @($repo,$out) -Test {
function Read-Source([string]$path){
 if($Baseline){$s=(& git -C $repo show "9099990c6:$path") -join "`n";if($LASTEXITCODE){throw "Cannot read pinned $path"};return $s}
 return [IO.File]::ReadAllText("$repo/$path")
}
function Block([string]$s,[string]$signature){
 $a=$s.IndexOf($signature,[StringComparison]::Ordinal);if($a -lt 0){throw "Missing $signature"}
 $b=$s.IndexOf('{',$a);$depth=1;$i=$b+1
 while($i -lt $s.Length -and $depth){if($s[$i] -eq '{'){$depth++}elseif($s[$i] -eq '}'){$depth--};$i++}
 if($depth){throw "Unclosed $signature"};return $s.Substring($a,$i-$a)
}
$smart=Read-Source 'src/server/game/AI/SmartScripts/SmartScript.cpp'
$a=$smart.IndexOf('        case SMART_ACTION_SET_DATA:');$b=$smart.IndexOf('        case SMART_ACTION_MOVE_FORWARD:',$a)
if($a -lt 0 -or $b -le $a){throw 'Missing actual SET_DATA handler'}
[IO.File]::WriteAllText("$out/SetData.inc",$smart.Substring($a,$b-$a))
$a=$smart.IndexOf('    if (e.link && e.link != e.event_id)');$b=$smart.IndexOf('void SmartScript::InstallTemplate(',$a)
if($a -lt 0 -or $b -le $a){throw 'Missing actual LINK tail'}
[IO.File]::WriteAllText("$out/LinkTail.inc",$smart.Substring($a,$b-$a))
$members=@((Block $smart 'bool SmartScript::IsCreature(WorldObject* obj)'),(Block $smart 'bool SmartScript::IsGameObject(WorldObject* obj)'),(Block (Read-Source 'src/server/game/Entities/Creature/Creature.cpp') 'bool Creature::AIM_Destroy()'),(Block (Read-Source 'src/server/game/Entities/GameObject/GameObject.cpp') 'GameObjectAI* GameObject::AI() const'))
[IO.File]::WriteAllLines("$out/NativeMembers.inc",[string[]]$members)
[IO.File]::WriteAllText("$out/CreatureAI.inc",(Block (Read-Source 'src/server/game/Entities/Creature/Creature.h') 'CreatureAI* AI() const'))
$header=Read-Source 'src/server/game/AI/SmartScripts/SmartScriptMgr.h'
$constants=@('SMART_ACTION_SET_DATA','SMART_EVENT_LINK')|ForEach-Object {$m=[regex]::Match($header,"$_\s*=\s*(\d+)");if(!$m.Success){throw "Missing $_"};"constexpr uint32 $_=$($m.Groups[1].Value);"}
[IO.File]::WriteAllLines("$out/Constants.inc",[string[]]$constants)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcVersion=([IO.File]::ReadAllText("$vs/VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt")).Trim()
$asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
[IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/SmartSetDataTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nexit /b %errorlevel%`n")
& "$out/build.cmd";if($LASTEXITCODE){throw 'SET_DATA native fixture compilation failed'}
function Run-Case([string]$name,[bool]$ExpectedFailure=$false){
 $start=New-Object Diagnostics.ProcessStartInfo
 $start.FileName="$out/test.exe";$start.Arguments=$name;$start.WorkingDirectory=$out
 $start.UseShellExecute=$false;$start.CreateNoWindow=$true;$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
 $start.EnvironmentVariables['PATH']="$vs/VC/Tools/MSVC/$vcVersion/bin/Hostx64/x64;$env:PATH"
 $p=New-Object Diagnostics.Process;$p.StartInfo=$start
 if(!$p.Start()){throw 'Cannot start owned fixture'}
 $pidOwned=$p.Id;$stdout=$p.StandardOutput.ReadToEndAsync();$stderr=$p.StandardError.ReadToEndAsync()
 if(!$p.WaitForExit(30000)){throw "Owned fixture $pidOwned did not exit within30s; inspect before cleanup"}
 $o=$stdout.Result;$e=$stderr.Result;$exit=$p.ExitCode;$p.Dispose()
 [IO.File]::WriteAllText("$out/$name.stdout.log",$o);[IO.File]::WriteAllText("$out/$name.stderr.log",$e)
 [IO.File]::WriteAllText("$out/$name.result.json",(@{Case=$name;PID=$pidOwned;Exit=$exit;ExpectedCounterfactual=$ExpectedFailure}|ConvertTo-Json))
 if($ExpectedFailure){
  if(!$AddressSanitizer){throw 'Counterfactual requires AddressSanitizer'}
  if($exit -eq 0 -or "$o$e" -notmatch 'AddressSanitizer: (DEADLYSIGNAL|access-violation|SEGV)' -or "$o$e" -notmatch 'SmartScript::ProcessAction' -or "$o$e" -notmatch 'address 0x0+ '){throw "Counterfactual did not reproduce native null dispatch: $name exit$exit; inspect $out"}
  "EXPECTED FAIL counterfactual $name : native absent-AI dispatch exits $exit with ASAN; retained $out/$name.*"
 }else{if($exit){throw "SET_DATA $name failed exit$exit; inspect $out"};$o;if($e){throw "Unexpected stderr for $name : $e"}}
}
Run-Case 'control'
if($Baseline){Run-Case 'destroyed-creature' $true;Run-Case 'missing-gameobject' $true}else{Run-Case 'regression'}
}
