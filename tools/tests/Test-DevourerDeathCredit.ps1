param([switch]$Baseline,[switch]$AddressSanitizer,[switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/devourer-death-credit"
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/$(if($Baseline){'baseline'}else{'current'})" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $path='src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp'
 $source=Get-Content "$repo/$path" -Raw
 $old=(& git -C $repo show "728285dc:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Devourer baseline unavailable'}
 foreach($pair in @(@('Zirus','class npc_devourer_zirus :'),@('Akaari','class npc_akaari_shadowgore_last :'))){
  $scope=Block $source $pair[1];$pinned=Block $old $pair[1];$death=Block $scope 'void JustDied(';$oldDeath=Block $pinned 'void JustDied('
  if(($scope.Replace($death,$oldDeath)-replace '\s','') -cne ($pinned-replace '\s','')){throw 'Unreviewed change outside death callbacks'}
  [IO.File]::WriteAllText("$out/$($pair[0])Death.inc",$(if($Baseline){$oldDeath}else{$death}))
 }
 $hostSource=Get-Content "$PSScriptRoot/KingslayersDeathCreditTest.cpp" -Raw
 $hostSource=$hostSource.Substring(0,$hostSource.IndexOf('struct Creature;')).Replace('asset==51615','(asset==50298||asset==50300)')
 $hostSource=$hostSource.Replace('bool playerType=false;', 'uint32 GetTypeId()const{return playerType?4u:3u;} bool playerType=false;')
 [IO.File]::WriteAllText("$out/OwnerHost.inc",$hostSource+'struct Creature;')
 [IO.File]::WriteAllText("$out/OwnerGuid.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'ObjectGuid GetCharmerOrOwnerGUID() const'))
 $unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
 [IO.File]::WriteAllText("$out/OwnerPlayer.inc",(Block $unit 'Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'))
 $first=$unit.IndexOf('    // Prevent killing unit twice',$unit.IndexOf('void Unit::Kill('));$last=$unit.IndexOf('    volatile uint32',$first)
 [IO.File]::WriteAllText("$out/DeathGuard.inc",$unit.Substring($first,$last-$first))
 $accessor=Get-Content "$repo/src/server/game/Globals/ObjectAccessor.h" -Raw
 [IO.File]::WriteAllText("$out/PlayerFind.inc",(Block $accessor 'static T* Find(ObjectGuid guid)'))
 [IO.File]::WriteAllText("$out/PlayerLookup.inc",(Block $accessor 'static Player* GetObjectInWorld(ObjectGuid guid, Player*')+(Block $accessor 'template<class T> static T* GetObjectInMap(')+'static Player* GetPlayer(WorldObject const&, ObjectGuid);')
 [IO.File]::WriteAllText("$out/PlayerLookupMethod.inc",(Block (Get-Content "$repo/src/server/game/Globals/ObjectAccessor.cpp" -Raw) 'Player* ObjectAccessor::GetPlayer('))
 [IO.File]::WriteAllText("$out/ScenarioGate.inc",(Block (Get-Content "$repo/src/server/game/Scenarios/Scenario.cpp" -Raw) 'bool Scenario::CanUpdateCriteria('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $definition=if($Baseline){'/DBASELINE'}else{''};if($AddressSanitizer){$definition+=' /fsanitize=address /Zi'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/DevourerDeathCreditTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Devourer death regression failed'}
 if($VerifyNativeCatalog){
  $previous=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile DevourerCombatCatalog.cpp -ProbeName DevourerCombatCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Devourer native catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
  $rows=Get-Content "$out/native.tsv"
  foreach($expected in @("STEP`t3`t2366`t47364","STEP`t5`t2368`t47368","CRITERION`t47365`t47364`t1`t0`t29789`t92`t50298","CRITERION`t47369`t47368`t1`t0`t29791`t92`t50300")){if($rows -notcontains $expected){throw 'Native Devourer death membership differs'}}
 }
}
