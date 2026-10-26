param([switch]$Baseline,[switch]$AddressSanitizer,[switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/dreadblades-brinebeard"
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/$(if($Baseline){'baseline'}else{'current'})" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $path='src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/Dreadblades.cpp'
 $source=Get-Content "$repo/$path" -Raw
 if($Baseline){$source=(& git -C $repo show "403d9516:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Brinebeard baseline missing'}}
 [IO.File]::WriteAllText("$out/Brinebeard.inc",(Block $source 'struct npc_lord_brinebeardAI : ScriptedAI')+';')
 $hostSource=Get-Content "$PSScriptRoot/KingslayersDeathCreditTest.cpp" -Raw
 $hostSource=$hostSource.Substring(0,$hostSource.IndexOf('struct Creature;')).Replace('51615','48531')
 $hostSource=$hostSource.Replace('struct Player;','struct Creature;struct Player;')
 $hostSource=$hostSource.Replace('bool playerType=false;', 'uint32 GetTypeId()const{return playerType?4u:3u;} uint32 Deal(Creature*,uint32); bool playerType=false;')
 $hostSource=$hostSource.Replace('    std::vector<uint32> criteria;',"    uint32 classId=4,status=3;bool phase=true;uint32 getClass()const{return classId;} uint32 GetQuestStatus(uint32 id){assert(id==40849);return status;} bool InSamePhase(Creature*)const{return phase;}`n    std::vector<uint32> criteria;")
 $hostSource=$hostSource.Replace('uint32 classId=4,status=3;', 'std::function<void()> onCredit;uint32 classId=4,status=3;').Replace('criteria.push_back(asset);','criteria.push_back(asset);if(onCredit)onCredit();')
 [IO.File]::WriteAllText("$out/OwnerHost.inc",$hostSource)
 [IO.File]::WriteAllText("$out/OwnerGuid.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'ObjectGuid GetCharmerOrOwnerGUID() const'))
 [IO.File]::WriteAllText("$out/OwnerPlayer.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw) 'Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'))
 $accessor=Get-Content "$repo/src/server/game/Globals/ObjectAccessor.h" -Raw
 [IO.File]::WriteAllText("$out/PlayerFind.inc",(Block $accessor 'static T* Find(ObjectGuid guid)'))
 [IO.File]::WriteAllText("$out/PlayerLookup.inc",(Block $accessor 'static Player* GetObjectInWorld(ObjectGuid guid, Player*')+(Block $accessor 'template<class T> static T* GetObjectInMap(')+'static Player* GetPlayer(WorldObject const&, ObjectGuid);')
 [IO.File]::WriteAllText("$out/PlayerLookupMethod.inc",(Block (Get-Content "$repo/src/server/game/Globals/ObjectAccessor.cpp" -Raw) 'Player* ObjectAccessor::GetPlayer('))
 [IO.File]::WriteAllText("$out/ScenarioGate.inc",(Block (Get-Content "$repo/src/server/game/Scenarios/Scenario.cpp" -Raw) 'bool Scenario::CanUpdateCriteria('))
 $core=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
 $first=$core.IndexOf('    damage /= victim->GetHealthMultiplierForTarget(this);');$bridge=Block $core.Substring($first) '    if (Creature* creature = victim->ToCreature())'
 if($first -lt 0 -or $core.IndexOf('    if (health <= damage)',$first) -lt $first+$bridge.Length){throw 'Native post-scaling/death hook order changed'}
 [IO.File]::WriteAllText("$out/ScaledHook.inc",'    damage /= victim->GetHealthMultiplierForTarget(this);'+$bridge)
 $eh=Get-Content "$repo/src/server/game/AI/EventMap.h" -Raw;$es=Get-Content "$repo/src/server/game/AI/EventMap.cpp" -Raw
 [IO.File]::WriteAllText("$out/EventMapClass.inc",(Block $eh 'enum c_events')+';'+(Block $eh 'class TC_GAME_API EventMap').Replace('TC_GAME_API ','')+';')
 [IO.File]::WriteAllText("$out/EventMapMethods.inc",((@('void EventMap::Reset(','void EventMap::ScheduleEvent(uint32 eventId, uint32 time','uint32 EventMap::ExecuteEvent(','void EventMap::CancelEvent(')|ForEach-Object {Block $es $_}) -join "`n"))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $definition=if($Baseline){'/DBASELINE'}else{''};if($AddressSanitizer){$definition+=' /fsanitize=address /Zi'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/DreadbladesBrinebeardTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Brinebeard regression failed'}
 if($VerifyNativeCatalog){
  $previous=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile DreadbladesCombatCatalog.cpp -ProbeName DreadbladesCombatCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Brinebeard native catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
  $rows=Get-Content "$out/native.tsv"
  foreach($expected in @("STEP`t2`t2132`t46670","CRITERION`t46671`t46670`t1`t0`t29460`t92`t48531")){if($rows -notcontains $expected){throw 'Native Brinebeard stage2 surrender membership differs'}}
 }
}
