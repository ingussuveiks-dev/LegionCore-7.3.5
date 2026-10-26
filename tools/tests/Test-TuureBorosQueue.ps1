param([switch]$Baseline,[switch]$AddressSanitizer)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/tuure-boros-queue"
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/$(if($Baseline){'baseline'}else{'current'})" -RuntimeDirectories @($repo,$out) -Test {
 $path='src/server/scripts/Scenario/Artifacts/Priest/Tuure/Tuure.cpp';$current=Get-Content "$repo/$path" -Raw;$source=$current
 if($Baseline){$source=(& git -C $repo show "473d023c:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Boros queue baseline missing'}}
 $actor=Block $source 'struct npc_vindicator_borosAI : ScriptedAI'
 $movement=Block $actor 'void MovementInform(';$first=$movement.IndexOf('case 1:');$last=$movement.IndexOf('case 2:',$first)
 if($first -lt 0 -or $last -le $first){throw 'Native point1 callback body missing'}
 $arrival=$movement.Substring($first+7,$last-$first-7);$break=$arrival.LastIndexOf('break;');if($break -lt 0){throw 'POINT1 ending missing'}
 [IO.File]::WriteAllText("$out/Arrival.inc","void ArrivedFirst(){`n"+$arrival.Substring(0,$break)+"`n}")
 [IO.File]::WriteAllText("$out/Methods.inc",((@('void Reset(','void MoveInLineOfSight(','void EnterEvadeMode(','void UpdateLifecycle(')|ForEach-Object {Block $actor $_}) -join "`n"))
 [IO.File]::WriteAllText("$out/Recover.inc",(Block $current 'namespace TuureCompanion'))
 $fh=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$fs=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
 [IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $fh 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$fs.Substring($fs.IndexOf('FunctionProcessor::FunctionProcessor()')))
 [IO.File]::WriteAllText("$out/Lifecycle.inc",(Block (Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw) 'if (i_AI || i_disabledAI)'))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $def=if($Baseline){'/DBASELINE'}else{''};if($AddressSanitizer){$def+=' /fsanitize=address /Zi'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $def /I`"$out`" `"$PSScriptRoot/TuureBorosQueueTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Boros queue lifetime regression failed'}
}
