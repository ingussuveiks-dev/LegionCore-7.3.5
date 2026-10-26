param([switch]$Baseline,[switch]$AddressSanitizer,[switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/dreadblades-death-credit"
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @("$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $path='src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/Dreadblades.cpp'
 $source=Get-Content "$repo/$path" -Raw
 $old=(& git -C $repo show "38f88905:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Eliza baseline missing'}
 $source=Block $source 'class npc_dread_admiral_eliza:'
 $old=Block $old 'class npc_dread_admiral_eliza:'
 $death=Block (Block $source 'struct npc_dread_admiral_elizaAI : ScriptedAI') 'void JustDied('
 $oldDeath=Block (Block $old 'struct npc_dread_admiral_elizaAI : ScriptedAI') 'void JustDied('
 if(($source.Replace($death,$oldDeath) -replace '\s','') -cne ($old -replace '\s','')){throw 'Unreviewed change outside Eliza death callback'}
 [IO.File]::WriteAllText("$out/ElizaDeath.inc",$(if($Baseline){$oldDeath}else{$death}))
 $hostSource=Get-Content "$PSScriptRoot/KingslayersDeathCreditTest.cpp" -Raw
 $hostSource=$hostSource.Substring(0,$hostSource.IndexOf('struct Creature;')).Replace('51615','48582')
 if($Baseline){$hostSource=$hostSource.Replace('bool playerType=false;', 'uint32 GetTypeId() const{return playerType?4u:3u;} bool playerType=false;')}
 [IO.File]::WriteAllText("$out/OwnerHost.inc",$hostSource+'struct Creature;')
 [IO.File]::WriteAllText("$out/OwnerGuid.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'ObjectGuid GetCharmerOrOwnerGUID() const'))
 [IO.File]::WriteAllText("$out/OwnerPlayer.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw) 'Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'))
 $accessor=Get-Content "$repo/src/server/game/Globals/ObjectAccessor.h" -Raw
 [IO.File]::WriteAllText("$out/PlayerFind.inc",(Block $accessor 'static T* Find(ObjectGuid guid)'))
 [IO.File]::WriteAllText("$out/PlayerLookup.inc",(Block $accessor 'static Player* GetObjectInWorld(ObjectGuid guid, Player*')+(Block $accessor 'template<class T> static T* GetObjectInMap(')+'static Player* GetPlayer(WorldObject const&, ObjectGuid);')
 [IO.File]::WriteAllText("$out/PlayerLookupMethod.inc",(Block (Get-Content "$repo/src/server/game/Globals/ObjectAccessor.cpp" -Raw) 'Player* ObjectAccessor::GetPlayer('))
 [IO.File]::WriteAllText("$out/ScenarioGate.inc",(Block (Get-Content "$repo/src/server/game/Scenarios/Scenario.cpp" -Raw) 'bool Scenario::CanUpdateCriteria('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $definition=if($Baseline){'/DBASELINE'}else{''};if($AddressSanitizer){$definition+=' /fsanitize=address /Zi'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/DreadbladesDeathCreditTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Eliza regression failed'}
 if($VerifyNativeCatalog){
  $previous=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile DreadbladesCombatCatalog.cpp -ProbeName DreadbladesCombatCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Eliza native catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
  $rows=Get-Content "$out/native.tsv"
  foreach($expected in @("STEP`t4`t2150`t46690","CRITERION`t46707`t46690`t1`t0`t29469`t92`t48582","CRITERION`t46691`t46690`t1`t0`t29463`t92`t48553")){if($rows -notcontains $expected){throw 'Eliza final death/pickup membership differs'}}
 }
}
