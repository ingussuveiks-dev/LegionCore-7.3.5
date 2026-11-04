param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/merlis-intro-asan/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $path='src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp';$current=Get-Content "$repo/$path" -Raw;$old=(& git -C $repo show "686d4d730:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Merlis predecessor missing'}
 # Garona reveal has its own whole-source/ASAN/native certificate.
 $current=$current.Replace((Block $current 'class npc_garona_halforcen_outro : public CreatureScript'),(Block $old 'class npc_garona_halforcen_outro : public CreatureScript'))
 $ai=Block $current 'struct npc_merlis_malaganAI : ScriptedAI';$prior=Block $old 'struct npc_merlis_malaganAI : ScriptedAI'
 # The independent source handoff suite certifies these corpse helpers and
 # summon/death changes; preserve the earlier intro/combat boundary exactly.
 $start=$ai.IndexOf('        ObjectGuid outroActor,');$tail=Block $ai 'void UpdateLifecycle(';$end=$ai.IndexOf($tail,$start)+$tail.Length
 if($start -lt 0 -or $end -lt 0){throw 'Certified handoff boundary missing'}
 $normalizedHandoff=$ai.Remove($start,$end-$start)
 foreach($name in @('void JustSummoned(','void JustDied(')){$normalizedHandoff=$normalizedHandoff.Replace((Block $ai $name),(Block $prior $name))}
 $current=$current.Replace($ai,$normalizedHandoff);$ai=$normalizedHandoff
 $a=$ai.IndexOf('        ObjectGuid introRecipient,');$b=$ai.IndexOf('        void Reset()',$a);if($a -lt 0 -or $b -lt 0){throw 'Dedicated Merlis intro boundary missing'}
 [IO.File]::WriteAllText("$out/MerlisHelpers.inc",$ai.Substring($a,$b-$a))
 $normalized=$ai.Remove($a,$b-$a);$update=Block $ai 'void UpdateAI('; $oldUpdate=Block $prior 'void UpdateAI('
 $normalized=$normalized.Replace($update,$oldUpdate)
 $withoutComments=[regex]::Replace($old,'(?s)/\* void MoveInLineOfSight.*?\*/','')
 if(($current.Replace($ai,$normalized)-replace '\s','') -cne ($withoutComments-replace '\s','')){throw 'Only dedicated Merlis helpers/intro dispatch and dead commented intro may change'}
 $expected='void UpdateAI(uint32 diff) override { UpdateIntro(diff);'+$oldUpdate.Substring($oldUpdate.IndexOf('if (!UpdateVictim())'))
 if(($update-replace '\s','') -cne ($expected-replace '\s','')){throw 'Original Merlis combat tail changed'}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/MerlisIntroTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Actual Merlis helper ASAN failed'}
}
