param([switch]$Baseline,[switch]$AddressSanitizer,[switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/devourer-capture-damage"
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/$(if($Baseline){'baseline'}else{'current'})" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $path='src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp';$source=Block (Get-Content "$repo/$path" -Raw) 'class npc_akaari_shadowgore :'
 $old=(& git -C $repo show "a0206501:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned capture baseline unavailable'};$old=Block $old 'class npc_akaari_shadowgore :'
 $guard=Block $source 'void DamageTakenAfterScaling('
 if(($source.Replace($guard,'')-replace '\s','') -cne ($old-replace '\s','')){throw 'Unexpected capture change outside final-damage guard'}
 [IO.File]::WriteAllText("$out/Guard.inc",$(if($Baseline){''}else{$guard}))
 [IO.File]::WriteAllText("$out/NativeDefault.inc",(Block (Get-Content "$repo/src/server/game/AI/CoreAI/UnitAI.h" -Raw) 'virtual void DamageTakenAfterScaling('))
 $hostSource=Get-Content "$PSScriptRoot/KingslayersDeathCreditTest.cpp" -Raw;$hostSource=$hostSource.Substring(0,$hostSource.IndexOf('struct Creature;')).Replace('51615','50261')
 $hostSource=$hostSource.Replace('struct Player;','struct Creature;struct Player;').Replace('bool playerType=false;', 'uint32 Deal(Creature*,uint32); bool playerType=false;')
 $hostSource=$hostSource.Replace('uint32 currentTree=48588,_criteriaTreeForUpdate=0;', 'uint32 id=1078,step=0;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;} uint32 currentTree=47352,_criteriaTreeForUpdate=0;')
 $hostSource=$hostSource.Replace('    std::vector<uint32> criteria;',"    uint32 classId=4,status=3;bool phase=true;uint32 getClass()const{return classId;} uint32 GetQuestStatus(uint32 id){assert(id==41924);return status;} bool InSamePhase(Creature*)const{return phase;}`n    std::vector<uint32> criteria;")
 [IO.File]::WriteAllText("$out/OwnerHost.inc",$hostSource)
 [IO.File]::WriteAllText("$out/OwnerGuid.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'ObjectGuid GetCharmerOrOwnerGUID() const'))
 $unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
 [IO.File]::WriteAllText("$out/OwnerPlayer.inc",(Block $unit 'Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'))
 $first=$unit.IndexOf('    damage /= victim->GetHealthMultiplierForTarget(this);');$bridge=Block $unit.Substring($first) '    if (Creature* creature = victim->ToCreature())'
 if($first -lt 0 -or $unit.IndexOf('    if (health <= damage)',$first) -lt $first+$bridge.Length){throw 'Native final-damage/death hook order changed'}
 [IO.File]::WriteAllText("$out/ScaledHook.inc",'    damage /= victim->GetHealthMultiplierForTarget(this);'+$bridge)
 $accessor=Get-Content "$repo/src/server/game/Globals/ObjectAccessor.h" -Raw
 [IO.File]::WriteAllText("$out/PlayerFind.inc",(Block $accessor 'static T* Find(ObjectGuid guid)'))
 [IO.File]::WriteAllText("$out/PlayerLookup.inc",(Block $accessor 'static Player* GetObjectInWorld(ObjectGuid guid, Player*')+(Block $accessor 'template<class T> static T* GetObjectInMap(')+'static Player* GetPlayer(WorldObject const&, ObjectGuid);')
 [IO.File]::WriteAllText("$out/PlayerLookupMethod.inc",(Block (Get-Content "$repo/src/server/game/Globals/ObjectAccessor.cpp" -Raw) 'Player* ObjectAccessor::GetPlayer('))
 [IO.File]::WriteAllText("$out/ScenarioGate.inc",(Block (Get-Content "$repo/src/server/game/Scenarios/Scenario.cpp" -Raw) 'bool Scenario::CanUpdateCriteria('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $def=if($Baseline){'/DBASELINE'}else{''};if($AddressSanitizer){$def+=' /fsanitize=address /Zi'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $def /I`"$out`" `"$PSScriptRoot/DevourerCaptureDamageTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native capture damage regression failed'}
 if($VerifyNativeCatalog){$prior=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile DevourerCombatCatalog.cpp -ProbeName DevourerCombatCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Capture native metadata failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$prior};$rows=Get-Content "$out/native.tsv";foreach($row in @("STEP`t0`t2363`t47352","CRITERION`t47353`t47352`t1`t0`t29781`t92`t50261")){if($rows -notcontains $row){throw 'Actual opening capture criterion changed'}}}
}
