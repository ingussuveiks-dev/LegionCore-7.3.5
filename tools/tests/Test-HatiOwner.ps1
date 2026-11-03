param([switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/hati-owner/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($repo,$out) -Test {
 function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
 function Shape([string]$s){(($s -replace '(?s)/\*.*?\*/','') -replace '//[^\n]*','') -replace '\s',''}
 $path='src/server/scripts/Spells/spell_hunter.cpp';$source=Get-Content "$repo/$path" -Raw;$old=(& git -C $repo show "ee6b845e4:$path") -join "`n";if($LASTEXITCODE){throw 'Missing pinned non-Player baseline'}
 $actor=Block $source 'class spell_hun_hatis_bond :';$prior=Block $old 'class spell_hun_hatis_bond :'
 if($actor.Contains('if (creature == caster->ToPlayer()->GetPet())')){
  & "$PSScriptRoot/Test-HatiPrimaryPet.ps1" -ScopeOnly
  $source=(& git -C $repo show "063ac15fa:$path") -join "`n";if($LASTEXITCODE){throw 'Certified pre-primary source missing'};$actor=Block $source 'class spell_hun_hatis_bond :'
 }
 $guard='if (!caster || !caster->IsAlive() || !caster->ToPlayer())'
 if(([regex]::Matches($actor,[regex]::Escape($guard))).Count -ne 1 -or (Shape $source.Replace($guard,'if (!caster || !caster->IsAlive())')) -cne (Shape $old)){throw 'Only actual Hati update Player admission may change; entire previous passive/lifecycle/other scripts retained'}
 if($ScopeOnly){'PASS exact non-Player guard before Hati controlled iteration/player-only lifecycle; full previous passive/apply/remove/timer/lookup/summon/cleanup and other scripts retained';return}
 & "$PSScriptRoot/Test-HatiReaction.ps1" -OutputDirectory "$out/retained"
 if(!$?){throw 'Retained Hati reaction native suite failed'}
 Get-ChildItem -LiteralPath "$out/retained" -Filter '*.inc' | Copy-Item -Destination $out
 $class='struct BondOwnerBaseline:AuraScript{uint32 update=0;'+(Block $prior 'void OnUpdate(')+(Block $prior 'void Register()').Replace('spell_hun_hatis_bond_AuraScript::','BondOwnerBaseline::')+'};'
 [IO.File]::WriteAllText("$out/OwnerBaseline.inc",$class)
 [IO.File]::WriteAllText("$out/PlayerPet.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'Pet* Player::GetPet() const'))
 [IO.File]::WriteAllText("$out/Load.inc",(Block (Get-Content "$repo/src/server/game/Spells/SpellScript.cpp" -Raw) 'bool AuraScript::_Load('))
 if((Shape (Block (Get-Content "$repo/src/server/game/Spells/SpellScript.h" -Raw) 'virtual bool Load()')) -cne (Shape 'virtual bool Load() { return true; }')){throw 'Native default aura admission changed'}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $vcVersion=([IO.File]::ReadAllText("$vs/VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt")).Trim()
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/HatiOwnerTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nexit /b %errorlevel%`n")
 & "$out/build.cmd";if($LASTEXITCODE){throw 'Hati owner native fixture compilation failed'}
 foreach($case in @('counterfactual','regression')){
  & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/cases/$case" -RuntimeDirectories @($out) -Test {
   $info=New-Object Diagnostics.ProcessStartInfo;$info.FileName="$out/test.exe";$info.Arguments=$case;$info.WorkingDirectory=$out
   $info.UseShellExecute=$false;$info.CreateNoWindow=$true;$info.RedirectStandardOutput=$true;$info.RedirectStandardError=$true
   $info.EnvironmentVariables['PATH']="$vs/VC/Tools/MSVC/$vcVersion/bin/Hostx64/x64;$env:PATH"
   $owned=New-Object Diagnostics.Process;$owned.StartInfo=$info;if(!$owned.Start()){throw 'Owned Hati owner child launch failed'}
   $ownedPid=$owned.Id;$stdout=$owned.StandardOutput.ReadToEndAsync();$stderr=$owned.StandardError.ReadToEndAsync()
   if(!$owned.WaitForExit(30000)){throw "Owned fixture $ownedPid requires investigation"}
   $exit=$owned.ExitCode;$o=$stdout.Result;$e=$stderr.Result;$owned.Dispose()
   [IO.File]::WriteAllText("$out/$case.stdout.log",$o);[IO.File]::WriteAllText("$out/$case.stderr.log",$e)
   [IO.File]::WriteAllText("$out/$case.result.json",(@{PID=$ownedPid;Exit=$exit;ExpectedCounterfactual=($case -eq 'counterfactual')}|ConvertTo-Json))
   if($case -eq 'counterfactual'){
    if(!$exit -or $e -notmatch 'AddressSanitizer: (access-violation|SEGV|DEADLYSIGNAL)' -or $e -notmatch 'Player::GetPet' -or $e -notmatch 'BondOwnerBaseline::OnUpdate'){throw 'Pinned native callback did not reproduce non-Player null-this access'}
    "EXPECTED FAIL pinnedee6b845e4: native Hati callback/native Player::GetPet null-this ASAN exit$exit; ordinary Player control passes in repaired matrix"
   }else{if($exit -or $e.Length){throw "Hati owner repaired suite failed exit$exit; inspect full stdout/stderr"};$o}
  }
 }
 if($VerifyReleaseBinary){
  if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$previous=$env:PYTHONPATH
  try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_hati_owner_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Hati owner Release check failed'}}finally{$env:PYTHONPATH=$previous}
 }
}
