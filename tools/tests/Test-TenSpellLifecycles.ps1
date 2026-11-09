param([Parameter(Mandatory=$true)][ValidateSet('Barding','Blessing','Rod','Undercurrent','Zen','T21','Whisper','Winning','Gumbo','Selfie')][string]$Case,[switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/ten-spell-lifecycles/$Case-$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
function Section([string]$s,[string]$start,[string]$end){$a=$s.IndexOf($start);$b=$s.IndexOf($end,$a);if($a -lt 0 -or $b -lt $a){throw "Missing section $start"};$s.Substring($a,$b-$a)}
function Extract-Class([string]$s,[string]$name){$a=$s.IndexOf("class $name ");if($a -lt 0){throw "Missing class $name"};$i=$s.IndexOf('{',$a);$depth=1;for($j=$i+1;$j -lt $s.Length;$j++){if($s[$j] -eq '{'){$depth++};if($s[$j] -eq '}'){$depth--};if(!$depth){return $s.Substring($a,$j+2-$a)}};throw 'Unclosed class'}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $plan=Get-Content "$PSScriptRoot/TenSpellLifecycles.json" -Raw|ConvertFrom-Json
 $entry=$plan|Where-Object key -eq $Case
 $path="src/server/scripts/Spells/$($entry.file)";$s=Get-Content "$repo/$path" -Raw -Encoding utf8
 if($Baseline){$s=(& git -C $repo show "c7f6e2f46:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned ten-fix baseline missing'}}
 [IO.File]::WriteAllText("$out/Selected.inc",(Extract-Class $s $entry.cls))
 $s=Get-Content "$repo/src/server/game/Spells/SpellScript.cpp" -Raw
 [IO.File]::WriteAllText("$out/Target.inc",(Section $s 'Unit* AuraScript::GetTarget() const' 'AuraApplication const* AuraScript::GetTargetApplication() const'))
 [IO.File]::WriteAllText("$out/Owner.inc",(Section $s 'Unit* AuraScript::GetUnitOwner() const' 'DynamicObject* AuraScript::GetDynobjOwner() const'))
 $s=Get-Content "$repo/src/server/game/Spells/SpellScript.h" -Raw
 [IO.File]::WriteAllText("$out/Hooks.inc",((Section $s 'enum SpellScriptState' '// helper class from which SpellScript')+(Section $s 'enum AuraScriptHookType' 'class TC_GAME_API AuraScript')))
 $s=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuras.cpp" -Raw
 [IO.File]::WriteAllText("$out/Dispatch.inc",(Section $s 'void Aura::CallScriptEffectUpdateHandlers(' 'void Aura::CallScriptEffectUpdatePeriodicHandlers('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $flags="/DCASE_$($Case.ToUpperInvariant()) /fsanitize=address /Zi";if($Baseline){$flags+=' /DCOUNTERFACTUAL'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /MD $flags /I`"$out`" `"$PSScriptRoot/TenSpellLifecyclesTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/compiler.pdb`"`nif %errorlevel% neq 0 exit /b %errorlevel%`n`"$out/test.exe`"`n")
 & "$out/run.cmd" *> "$out/run.log";$code=$LASTEXITCODE;Get-Content "$out/run.log" -Tail 12
 $expectedCrash=$Baseline -and $Case -in 'Barding','Blessing','Rod','Zen','Whisper','Winning'
 if($expectedCrash){$log=Get-Content "$out/run.log" -Raw;if(!$code -or $log -notmatch 'AddressSanitizer: (access-violation|SEGV|DEADLYSIGNAL)'){throw "Expected original null/nonplayer dereference not reproduced: $Case exit=$code"};"COUNTERFACTUAL $Case original ASAN null/nonplayer dereference,exit=$code;must be paired with repaired pass"}elseif($code){throw "$Case source regression failed: $code ($out/run.log)"}
}
# An explicitly verified counterfactual is a successful test assertion; its
# original nonzero executable exit remains recorded in run.log and the output.
$global:LASTEXITCODE=0
