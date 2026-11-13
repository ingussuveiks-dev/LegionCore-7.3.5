param([switch]$AddressSanitizer,[switch]$VerifyNativeCatalog,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory="")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/warswords-bonfire"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
$s=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Warrior/Warswords/Warswords.cpp" -Raw
[IO.File]::WriteAllText("$out/Bonfire.inc",(Block $s 'class go_mystic_bonfire : public GameObjectScript')+';')
$s=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
$a=$s.IndexOf('    if (gameObjTarget && m_spellInfo->Id == 198513');$b=$s.IndexOf('    // Tuure', $a);if($a -lt 0 -or $b -le $a){throw 'Missing native bonfire dispatch'}
[IO.File]::WriteAllText("$out/Dispatch.inc",$s.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$instrument=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $instrument /I`"$out`" `"$PSScriptRoot/WarswordsBonfireTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Bonfire original-method regression failed'}

if($VerifyNativeCatalog){
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile WarswordsBonfireCatalog.cpp -ProbeName WarswordsBonfireCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native bonfire data reader failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 $rows=Get-Content "$out/native.tsv"
 foreach($row in @("LOCK`t2474`t1`t3`t198513`t0`t0","SPELL`t198513`tLighting Bonfire","EFFECT`t198513`t0`t0`t33`t5`t23`t0","STEP`t1888`t45848","CRITERIA`t45849`t29136`t92`t47272`t0")){if($rows -notcontains $row){throw "Matching native bonfire data differs: $row"}}
 if(!($rows|Where-Object {$_ -match "^LOS_ATTRIBUTE`t198513`t0`t[0-9]+`t1$"}) -or !($rows|Where-Object {$_ -match "^LOS_ATTRIBUTE`t3365`t0`t[0-9]+`t0$"})){throw 'Native Lighting Bonfire LOS exemption or ordinary Opening contrast differs'}
 'PASS actual26972 Lock2474 requires198513; native Opening effect33/target23 and scenario944/1888/tree45849/event47272 retained'
}
if($VerifyReleaseBinary){
 if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_warswords_bonfire_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Selected Release bonfire proof failed'}}finally{$env:PYTHONPATH=$previous}
}
