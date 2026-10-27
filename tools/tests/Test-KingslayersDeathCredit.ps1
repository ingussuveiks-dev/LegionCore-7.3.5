param([switch]$Baseline,[switch]$AddressSanitizer,[switch]$VerifyNativeCatalog,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory="$PSScriptRoot/../../build-extractors/bin/Release")
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify repaired Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/kingslayers-death-credit"
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @("$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $path='src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp'
 $source=Get-Content "$repo/$path" -Raw -Encoding UTF8
 $pinned=(& git -C $repo show "ab20ad22:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned baseline unavailable'}
 $currentAI=Block $source 'struct npc_merlis_malaganAI : ScriptedAI';$oldAI=Block $pinned 'struct npc_merlis_malaganAI : ScriptedAI'
 $marker='void JustDied(Unit* who) override';$currentDeath=Block $currentAI $marker;$oldDeath=Block $oldAI $marker
 # Other Kingslayers actors and the pickup have independent regression suites.
 if(($currentAI.Replace($currentDeath,$oldDeath) -replace '\s','') -cne ($oldAI -replace '\s','')){throw 'Unreviewed change elsewhere in Merlis AI'}
 [IO.File]::WriteAllText("$out/MerlisDeath.inc",$(if($Baseline){$oldDeath}else{$currentDeath}))
 [IO.File]::WriteAllText("$out/OwnerGuid.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw -Encoding UTF8) 'ObjectGuid GetCharmerOrOwnerGUID() const'))
 [IO.File]::WriteAllText("$out/OwnerPlayer.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw -Encoding UTF8) 'Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'))
 $accessor=Get-Content "$repo/src/server/game/Globals/ObjectAccessor.h" -Raw -Encoding UTF8
 [IO.File]::WriteAllText("$out/PlayerFind.inc",(Block $accessor 'static T* Find(ObjectGuid guid)'))
 [IO.File]::WriteAllText("$out/PlayerLookup.inc",(Block $accessor 'static Player* GetObjectInWorld(ObjectGuid guid, Player*')+(Block $accessor 'template<class T> static T* GetObjectInMap(')+'static Player* GetPlayer(WorldObject const&, ObjectGuid);')
 [IO.File]::WriteAllText("$out/PlayerLookupMethod.inc",(Block (Get-Content "$repo/src/server/game/Globals/ObjectAccessor.cpp" -Raw -Encoding UTF8) 'Player* ObjectAccessor::GetPlayer('))
 [IO.File]::WriteAllText("$out/ScenarioGate.inc",(Block (Get-Content "$repo/src/server/game/Scenarios/Scenario.cpp" -Raw -Encoding UTF8) 'bool Scenario::CanUpdateCriteria('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $definition=if($Baseline){'/DBASELINE'}else{''};if($AddressSanitizer){$definition+=' /fsanitize=address /Zi'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/KingslayersDeathCreditTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Kingslayers death regression failed'}
 if($VerifyNativeCatalog){
  $previous=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile KingslayersCombatCatalog.cpp -ProbeName KingslayersCombatCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native Kingslayers combat catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
  $rows=Get-Content "$out/native.tsv" -Encoding UTF8
  foreach($expected in @("STEP`t6`t2561`t48586","STEP`t7`t2562`t48588","STEP`t8`t2563`t48590","CRITERION`t48587`t48586`t1`t0`t30474`t92`t51614","CRITERION`t48589`t48588`t1`t0`t30475`t92`t51615","CRITERION`t48591`t48590`t1`t0`t30476`t92`t51616")){if($rows -notcontains $expected){throw 'Native Merlis stage7 death/pickup neighbors changed'}}
  'PASS actual26972/read-only hotfix:51615=type92/30475/leaf48589/amount1/flags0 at1123 stage7; preceding51614 and following51616 remain distinct'
 }
 if($VerifyReleaseBinary){
  $previous=$env:PYTHONPATH;$previousErrors=$ErrorActionPreference
  try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";$ErrorActionPreference='Continue';& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_kingslayers_death_release.py" $ReleaseDirectory *> "$out/pe-check.log";$code=$LASTEXITCODE;$ErrorActionPreference=$previousErrors;Get-Content "$out/pe-check.log" -Encoding UTF8;if($code){throw 'Selected Merlis death repair missing'}}finally{$env:PYTHONPATH=$previous;$ErrorActionPreference=$previousErrors}
 }
}
