param([switch]$Baseline,[switch]$AddressSanitizer,[switch]$VerifyNativeCatalog,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory="$PSScriptRoot/../../build-extractors/bin/Release")
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify repaired Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/pet-follow-formation"
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @("$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $path='src/server/game/Entities/Unit/Unit.cpp'
 $current=Get-Content "$repo/$path" -Raw -Encoding UTF8
 $pinned=(& git -C $repo show "6cdf37b6:$path") -join "`n"
 if($LASTEXITCODE){throw 'Pinned formation baseline unavailable'}
 function Formation([string]$source){$method=Block $source 'void Unit::SetMinion('; $start=$method.IndexOf('    std::vector<ObjectGuid> followOrder');if($start -lt 0){$start=$method.IndexOf('    if (m_Controlled.size() < 5)')};if($start -lt 0){throw 'Missing native formation'};$method.Substring($start,$method.Length-$start-1)}
 $currentFormation=Formation $current;$pinnedFormation=Formation $pinned
 if(($current.Replace($currentFormation,$pinnedFormation) -replace '\s','') -cne ($pinned -replace '\s','')){throw 'Change outside SetMinion formation not reviewed'}
 $selected=if($Baseline){$pinnedFormation}else{$currentFormation}
 [IO.File]::WriteAllText("$out/Formation.inc",'void Owner::AssignFormation(Minion* minion,bool apply){'+$selected+'}')
 $guid=Get-Content "$repo/src/server/game/Entities/Object/ObjectGuid.cpp" -Raw -Encoding UTF8
 [IO.File]::WriteAllText("$out/GuidOperators.inc",(Block $guid 'bool ObjectGuid::operator==(')+(Block $guid 'bool ObjectGuid::operator<('))
 [IO.File]::WriteAllText("$out/PetReturn.inc",(Block (Get-Content "$repo/src/server/game/AI/CoreAI/PetAI.cpp" -Raw -Encoding UTF8) 'void PetAI::HandleReturnMovement('))
 $constants=Get-Content "$repo/src/server/game/Entities/Pet/PetDefines.h" -Encoding UTF8 | Where-Object {$_ -match '^#define PET_FOLLOW_(DIST|ANGLE)\s'}
 if($constants.Count -ne 2){throw 'Native follow constants unavailable'}
 [IO.File]::WriteAllText("$out/PetFollowConstants.inc",($constants -join "`n"))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $definition=if($Baseline){'/DBASELINE'}else{''};if($AddressSanitizer){$definition+=' /fsanitize=address /Zi'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/PetFollowFormationTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native formation regression failed'}
 if($VerifyNativeCatalog){
  $previous=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile HatiCompanionCatalog.cpp -ProbeName HatiCompanionCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Hati native catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
  $expected=@("SPELL`t197344`tHati's Bond","SPELL`t197388`tStormbound","SPELL`t211145`tStormbound","SPELL`t211146`tStormbound","SPELL`t211147`tStormbound","SPELL`t211148`tStormbound","SUMMON`t197388`t100324`t3725","SUMMON`t211145`t106548`t3725","SUMMON`t211146`t106549`t3725","SUMMON`t211147`t106550`t3725","SUMMON`t211148`t106551`t3725","PROPERTIES`t3725`t1`t1`t0`t134744578")
  if(Compare-Object $expected (Get-Content "$out/native.tsv" -Encoding UTF8)){throw 'Native Hati companion references changed'}
  'PASS native26972/read-only hotfix Hati Bond and five actual companion summons/properties3725'
 }
 if($VerifyReleaseBinary){
  $previous=$env:PYTHONPATH;$previousErrors=$ErrorActionPreference
  try{
   $env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";$ErrorActionPreference='Continue'
   & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_pet_follow_formation_release.py" $ReleaseDirectory *> "$out/pe-check.log"
   $code=$LASTEXITCODE;$ErrorActionPreference=$previousErrors;Get-Content "$out/pe-check.log" -Encoding UTF8
   if($code){throw 'Linked formation implementation missing; see pe-check.log'}
  }finally{$env:PYTHONPATH=$previous;$ErrorActionPreference=$previousErrors}
 }
}
