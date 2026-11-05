param([switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/akaari-opening-asan/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp" -Raw
 . "$PSScriptRoot/AkaariOpeningCertificate.ps1"
 [void](Restore-AkaariOpeningBoundary $source)
 $ai=Block (Block $source 'class npc_akaari_shadowgore :') 'struct npc_akaari_shadowgoreAI :'
 $selected='struct AkaariOpeningIntroAI : ScriptedAI { explicit AkaariOpeningIntroAI(Creature* c):ScriptedAI(c){} bool introDone=false,capturePending=false,captureComplete=false;'
 $selected+=$ai.Substring($ai.IndexOf('uint32 introTimer'),$ai.IndexOf('void PresentCapture(')-$ai.IndexOf('uint32 introTimer'))
 $reset=Block $ai 'void Reset('
 $selected+=$reset.Substring(0,$reset.IndexOf('events.Reset();'))+'}'
 $selected+=Block $ai 'void MoveInLineOfSight('
 $update=Block $ai 'void UpdateAI('
 $selected+=$update.Substring(0,$update.IndexOf('if (captureComplete)'))+'} };'
 [IO.File]::WriteAllText("$out/AkaariAI.inc",$selected)
 $scope=Get-Content "$repo/src/server/game/Entities/Object/QuestObjectScope.h" -Raw
 [IO.File]::WriteAllText("$out/QuestScope.inc",(Block $scope 'class QuestCancellationReceipt')+';'+(Block $scope "`nstruct QuestObjectScope")+';')
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/AkaariOpeningTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b %errorlevel%`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd" > "$out/stdout.log" 2> "$out/stderr.log";$code=$LASTEXITCODE;Get-Content "$out/stdout.log";if($code -or (Get-Item "$out/stderr.log").Length){Get-Content "$out/stderr.log" -Tail 12;throw "AkaariOpening intro ASAN exit=$code or stderr"}
 if($VerifyNativeCatalog){
  $previous=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf"
   & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile AkaariOpeningCatalog.cpp -ProbeName AkaariOpeningCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log"
   if(!$? -or $LASTEXITCODE){throw "Native opening3952/broadcast/audio catalog failed: $out/catalog.log"}
   Get-Content "$out/native.tsv"
  }finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 }
}
