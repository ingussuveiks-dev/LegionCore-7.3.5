param([switch]$VerifyLiveData,[switch]$VerifyNativeCatalog,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/artifact-resource";$data="$repo/build-extractors/runtime-test/artifact-resource-data"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($data,"$repo/build-extractors/runtime-test/script-pet-credit","$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 & "$PSScriptRoot/Test-ArtifactResourceData.ps1" -VerifyLiveData:$VerifyLiveData
 & "$PSScriptRoot/Test-ScriptPetCredit.ps1" -InvokerCastRowsPath "$data/row.tsv" -BaselineInvokerCastRowsPath "$data/baseline-row.tsv" -AddressSanitizer
 function Section([string]$source,[string]$start,[string]$end){$a=$source.IndexOf($start);$b=$source.IndexOf($end,[Math]::Max(0,$a));if($a -lt 0 -or $b -le $a){throw "Missing native section $start"};$source.Substring($a,$b-$a)}
 $source=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
 [IO.File]::WriteAllText("$out/Energize.inc",(Section $source 'void Spell::EffectEnergize(' 'void Spell::EffectEnergizePct('))
 $source=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuraEffects.cpp" -Raw
 [IO.File]::WriteAllText("$out/ProgressBar.inc",(Section $source 'void AuraEffect::HandleProgressBar(' 'void AuraEffect::HandleAuraStrangulate('))
 $source=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
 [IO.File]::WriteAllText("$out/ExplicitTarget.inc",(Section $source 'void Spell::SelectImplicitTargetObjectTargets(' 'void Spell::SelectImplicitChainTargets('))
 $source=Get-Content "$repo/src/server/game/Spells/SpellInfo.cpp" -Raw
 if($source -notmatch 'TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,\s+TARGET_DIR_NONE\},\s+// 92 TARGET_UNIT_SELECTED'){throw 'Native92 explicit unit semantics differ'}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/ArtifactResourceTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif not %errorlevel%==0 exit /b %errorlevel%`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Nexus energy regression failed'}
 if($VerifyNativeCatalog){$old=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ArtifactResourceCatalog.cpp -ProbeName ArtifactResourceCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$old}
  $native=Get-Content "$out/native.tsv"
  foreach($line in @("EFFECT`t152435`t0`t0`t30`t0`t10`t0`t0`t92`t0`t0`t10`t0","MISC`t152435`t0`t0`t0`t135`t100`t100","NEXUS_BAR`t371`t0`t0`t100","NEXUS_BAR`t376`t0`t0`t100","EFFECT`t207066`t0`t0`t6`t346`t371`t0`t0`t1`t0`t0`t0`t0","EFFECT`t211691`t0`t0`t6`t346`t376`t0`t0`t1`t0`t0`t0`t0")){if($native -cnotcontains $line){throw "Native Nexus energy catalog differs: $line"}}
 }
 if($VerifyReleaseBinary){$old=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_smart_player_cast_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Native opt-in caster Release path differs'}}finally{$env:PYTHONPATH=$old}}
}
