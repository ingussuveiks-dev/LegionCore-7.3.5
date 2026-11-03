param([switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/hati-primary/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($repo,$out) -Test {
 function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
 function Shape([string]$s){(($s -replace '(?s)/\*.*?\*/','') -replace '//[^\n]*','') -replace '\s',''}
 $path='src/server/scripts/Spells/spell_hunter.cpp';$source=Get-Content "$repo/$path" -Raw;$old=(& git -C $repo show "063ac15fa:$path") -join "`n";if($LASTEXITCODE){throw 'Missing pinned primary-pet baseline'}
 $actor=Block $source 'class spell_hun_hatis_bond :';$prior=Block $old 'class spell_hun_hatis_bond :'
 if($source.Contains('if (!caster->ToPlayer() || !caster->ToPlayer()->GetPet())')){
  & "$PSScriptRoot/Test-HatiResurrection.ps1" -ScopeOnly
  $source=(& git -C $repo show "fbaa14ff8:$path") -join "`n";if($LASTEXITCODE){throw 'Certified pre-resurrection source missing'};$actor=Block $source 'class spell_hun_hatis_bond :'
 }
 $check='if (creature == caster->ToPlayer()->GetPet())'
 if(([regex]::Matches($actor,[regex]::Escape($check))).Count -ne 1 -or (Shape $source.Replace($check,'')) -cne (Shape $old)){throw 'Only actual-primary identity check may change; previous complete Hati and all other scripts retained'}
 if($ScopeOnly){'PASS exact primary identity successor; previous Player guard/passive/apply/remove/cleanup retained';return}
 & "$PSScriptRoot/Test-HatiOwner.ps1" -OutputDirectory "$out/retained"
 if(!$?){throw 'Retained Hati owner/reaction suite failed'}
 Get-ChildItem -LiteralPath "$out/retained" -Filter '*.inc' | Copy-Item -Destination $out
 $class='struct BondPrimaryBaseline:AuraScript{uint32 update=0;'+(Block $prior 'void OnUpdate(')+(Block $prior 'void Register()').Replace('spell_hun_hatis_bond_AuraScript::','BondPrimaryBaseline::')+'};'
 [IO.File]::WriteAllText("$out/PrimaryBaseline.inc",$class)
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/HatiPrimaryPetTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/build.cmd";if($LASTEXITCODE){throw 'Hati actual primary-pet regression failed'}
 if($VerifyReleaseBinary){
  if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$previous=$env:PYTHONPATH
  try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_hati_primary_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Hati primary Release check failed'}}finally{$env:PYTHONPATH=$previous}
 }
}
