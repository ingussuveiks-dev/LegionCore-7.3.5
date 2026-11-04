param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/merlis-handoff-asan/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 . "$PSScriptRoot/MerlisCombatVoiceCertificate.ps1"
 $source=Restore-MerlisCombatVoiceBoundary (Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp" -Raw);$ai=Block $source 'struct npc_merlis_malaganAI : ScriptedAI'
 $pinned=(& git -C $repo show 'cf1f7a103:src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned handoff source certificate missing'}
 $prior=Block $pinned 'struct npc_merlis_malaganAI : ScriptedAI'
 $garona=Block $source 'class npc_garona_halforcen_outro :';$receipt=Block $garona 'uint32 GetData('
 if(($receipt-replace '\s','') -cne 'uint32GetData(uint32type)constoverride{returntype==108222&&introDone;}'){throw 'Unreviewed reveal receipt'}
 if(($source.Replace($ai,$prior).Replace($receipt,'')-replace '\s','') -cne ($pinned-replace '\s','')){throw 'Unreviewed change outside handoff/receipt'}
 $beforeDeath=Block $prior 'void JustDied('; $afterDeath=Block $ai 'void JustDied('
 $first=$beforeDeath.IndexOf('            if (Creature* oldgarona');$last=$beforeDeath.IndexOf('            if (player)',$first)
 $spawn=$beforeDeath.Substring($first,$last-$first)
 if(($afterDeath.Replace('            StartOutro();',$spawn)-replace '\s','') -cne ($beforeDeath-replace '\s','')){throw 'Owner credit/death cleanup changed outside source handoff'}
 $a=$ai.IndexOf('        ObjectGuid outroActor,');$tail=Block $ai 'void UpdateLifecycle(';$b=$ai.IndexOf($tail,$a)+$tail.Length;if($a -lt 0 -or $b -lt 0){throw 'Actual handoff boundary absent'}
 [IO.File]::WriteAllText("$out/Handoff.inc",$ai.Substring($a,$b-$a))
 foreach($pair in @(@('Death','void JustDied('),@('Reset','void Reset('),@('Summoned','void JustSummoned('))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $ai $pair[1]))}
 [IO.File]::WriteAllText("$out/GaronaReceipt.inc",(Block (Block $source 'class npc_garona_halforcen_outro :') 'uint32 GetData('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/MerlisHandoffTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b %errorlevel%`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd" > "$out/stdout.log" 2> "$out/stderr.log";$code=$LASTEXITCODE;Get-Content "$out/stdout.log";if($code -or (Get-Item "$out/stderr.log").Length){Get-Content "$out/stderr.log" -Tail 20;throw "Handoff ASAN exit=$code or stderr"}
}
