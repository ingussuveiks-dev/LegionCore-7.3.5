param([Parameter(Mandatory=$true)][ValidateSet('Umbral','Shatter','Flask','Comet','Flurry','Storm','Stomp','Wailing','Cookie','Dispel')][string]$Case,[switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/next-ten-spell-repairs/$Case-$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
function Section([string]$s,[string]$start,[string]$end){$a=$s.IndexOf($start);$b=$s.IndexOf($end,$a);if($a -lt 0 -or $b -lt $a){throw "Missing section $start"};$s.Substring($a,$b-$a)}
function Extract-Class([string]$s,[string]$name){$a=$s.IndexOf("class $name ");if($a -lt 0){throw "Missing class $name"};$i=$s.IndexOf('{',$a);$depth=1;for($j=$i+1;$j -lt $s.Length;$j++){if($s[$j] -eq '{'){$depth++};if($s[$j] -eq '}'){$depth--};if(!$depth){return $s.Substring($a,$j+2-$a)}};throw 'Unclosed class'}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $plan=Get-Content "$PSScriptRoot/NextTenSpellRepairs.json" -Raw|ConvertFrom-Json
 $entry=$plan|Where-Object key -eq $Case
 $path="src/server/scripts/Spells/$($entry.file)";$s=Get-Content "$repo/$path" -Raw -Encoding utf8
 if($Baseline){$s=(& git -C $repo show "50b42e107:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned ten-fix baseline missing'}}
 [IO.File]::WriteAllText("$out/Selected.inc",(Extract-Class $s $entry.cls))
 $s=Get-Content "$repo/src/server/game/Spells/SpellScript.cpp" -Raw
 [IO.File]::WriteAllText("$out/Target.inc",(Section $s 'Unit* AuraScript::GetTarget() const' 'AuraApplication const* AuraScript::GetTargetApplication() const'))
 [IO.File]::WriteAllText("$out/Owner.inc",(Section $s 'Unit* AuraScript::GetUnitOwner() const' 'DynamicObject* AuraScript::GetDynobjOwner() const'))
 $s=Get-Content "$repo/src/server/game/Spells/SpellScript.h" -Raw
 [IO.File]::WriteAllText("$out/Hooks.inc",((Section $s 'enum SpellScriptState' '// helper class from which SpellScript')+(Section $s 'enum AuraScriptHookType' 'class TC_GAME_API AuraScript')))
 $s=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuras.cpp" -Raw
 [IO.File]::WriteAllText("$out/Dispatch.inc",(Section $s 'void Aura::CallScriptEffectUpdateHandlers(' 'void Aura::CallScriptEffectUpdatePeriodicHandlers('))
 $core=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuras.cpp" -Raw
 function Block([string]$t,[string]$start){$a=$t.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$t.IndexOf('{',$a)+1;$d=1;while($d){$d+=([int]($t[$b] -eq '{'))-([int]($t[$b] -eq '}'));++$b};$t.Substring($a,$b-$a)}
 [IO.File]::WriteAllText("$out/CalcDispatch.inc",(Block $core 'void Aura::CallScriptEffectCalcAmountHandlers('))
 $header=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$impl=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
 [IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $header 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$impl.Substring($impl.IndexOf('FunctionProcessor::FunctionProcessor()')))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $flags="/DCASE_$($Case.ToUpperInvariant()) /fsanitize=address /Zi";if($Baseline){$flags+=' /DCOUNTERFACTUAL'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /MD $flags /I`"$out`" `"$PSScriptRoot/NextTenSpellRepairsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/compiler.pdb`"`nif %errorlevel% neq 0 exit /b %errorlevel%`n`"$out/test.exe`"`n")
 & "$out/run.cmd" *> "$out/run.log";$code=$LASTEXITCODE;Get-Content "$out/run.log" -Tail 12
 $expectedCrash=$Baseline -and $Case -in 'Shatter','Cookie'
 if($expectedCrash){$log=Get-Content "$out/run.log" -Raw;if(!$code -or $log -notmatch 'AddressSanitizer: (access-violation|SEGV|DEADLYSIGNAL)'){throw "Expected original null/nonplayer dereference not reproduced: $Case exit=$code"};"COUNTERFACTUAL $Case original ASAN null/nonplayer dereference,exit=$code;must be paired with repaired pass"}elseif($Baseline -and $Case -eq 'Flask'){if(!$code -or (Get-Content "$out/run.log" -Raw) -notmatch 'Assertion failed.*hi>=lo'){throw 'Pinned empty Flask choice not reproduced'};'COUNTERFACTUAL original Monk choice reaches invalid native RNG bounds;not successful gameplay'}elseif($code){throw "$Case source regression failed: $code ($out/run.log)"}
}
# An explicitly verified counterfactual is a successful test assertion; its
# original nonzero executable exit remains recorded in run.log and the output.
$global:LASTEXITCODE=0
