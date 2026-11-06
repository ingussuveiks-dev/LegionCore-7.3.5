param([switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/kingslayers-lucas-lifetime/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $path='src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp';$source=Get-Content "$repo/$path" -Raw
 if($Baseline){$source=(& git -C $repo show "3cf03e7a9:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Lucas baseline missing'}}else{. "$PSScriptRoot/KingslayersLucasCertificate.ps1";[void](Restore-KingslayersLucasBoundary $source)}
 $ai=Block $source 'struct npc_kingslayers_small_eventsAI :';$los=Block $ai 'void MoveInLineOfSight('; $branch=Block $los 'if (me->GetEntry() == 108399 &&';$begin=$los.IndexOf('if (introDone)');$end=$los.IndexOf($branch)+$branch.Length
 [IO.File]::WriteAllText("$out/LucasLos.inc",$los.Substring($begin,$end-$begin));if(!$Baseline){[IO.File]::WriteAllText("$out/LucasUpdate.inc",(Block $ai 'void UpdateLucas(uint32 diff)'))}
 $header=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$methods=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
 [IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64,std::function<void()>> FunctionList;'+(Block $header 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$methods.Substring($methods.IndexOf('FunctionProcessor::FunctionProcessor()')))
 $header=Get-Content "$repo/src/server/game/AI/EventMap.h" -Raw;$methods=Get-Content "$repo/src/server/game/AI/EventMap.cpp" -Raw
 [IO.File]::WriteAllText("$out/Events.inc",(Block $header 'enum c_events')+';'+(Block $header 'class TC_GAME_API EventMap').Replace('TC_GAME_API ','')+';'+((@('void EventMap::Reset(','void EventMap::ScheduleEvent(uint32 eventId, uint32 time','uint32 EventMap::ExecuteEvent(','void EventMap::CancelEvent(')|ForEach-Object {Block $methods $_}) -join "`n"))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath;$definition=if($Baseline){'/DLUCAS_BASELINE'}else{''}
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /fsanitize=address /Zi /Od /W4 $definition /I`"$out`" `"$PSScriptRoot/KingslayersLucasLifetimeTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nexit /b %errorlevel%`n")
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncd /d `"$out`"`n`"$out/test.exe`" %*`nexit /b %errorlevel%`n")
 & "$out/build.cmd" > "$out/build.log" 2> "$out/build-stderr.log";if($LASTEXITCODE -or (Get-Item "$out/build-stderr.log").Length){Get-Content "$out/build.log" -Tail 10;throw 'Lucas ASAN host build failed'}
 Push-Location $out
 try{
  & "$out/run.cmd" > "$out/stdout.log" 2> "$out/stderr.log";$code=$LASTEXITCODE;[IO.File]::WriteAllText("$out/ordinary-exit.txt",[string]$code);if($code -or (Get-Item "$out/stderr.log").Length){Get-Content "$out/stderr.log" -Tail 12;throw "Lucas ordinary extracted lifetime checks failed, exit=$code"};Get-Content "$out/stdout.log"
  & "$out/run.cmd" destroy > "$out/destroy-stdout.log" 2> "$out/destroy-stderr.log";$code=$LASTEXITCODE;[IO.File]::WriteAllText("$out/destroy-exit.txt",[string]$code)
  if($Baseline){if(!$code -or !(Select-String -LiteralPath "$out/destroy-stderr.log" -SimpleMatch 'ERROR: AddressSanitizer: heap-use-after-free')){throw 'Pinned freed-AI failure was not reproduced'};'COUNTERFACTUAL: pinned exact Lucas branch/native FunctionProcessor dereferences freed AI at5s; explicit ASAN failure paired with current passing regression, not ordinary success.'}else{if($code -or (Get-Item "$out/destroy-stderr.log").Length){throw 'Destroyed current AI retained an unsafe callback'};Get-Content "$out/destroy-stdout.log"}
 }finally{Pop-Location}
}
