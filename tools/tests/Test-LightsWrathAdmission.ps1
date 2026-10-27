param([string]$MigrationPath="$PSScriptRoot/../../sql/updates/world/2026_10_27_02_lights_wrath_pickup_admission.sql",[switch]$AddressSanitizer,[switch]$VerifyLive,[switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/lights-wrath-admission"
New-Item -ItemType Directory -Force $out|Out-Null
function Section([string]$s,[string]$start,[string]$end){$a=$s.IndexOf($start);$b=$s.IndexOf($end,[Math]::Max(0,$a));if($a -lt 0 -or $b -le $a){throw "Native section missing $start"};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo,$out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $migration=Get-Content $MigrationPath -Raw
 $filters=[ordered]@{gameobject_template='entry=248572';gameobject_loot_template='Entry=248572';smart_scripts='entryorguid=248572 AND source_type=1';conditions='(SourceTypeOrReferenceId=4 AND SourceGroup=248572) OR (SourceTypeOrReferenceId=17 AND SourceEntry=208011) OR (SourceTypeOrReferenceId=22 AND SourceEntry=248572 AND SourceId=1)'}
 $setup='';foreach($t in $filters.Keys){$setup+="CREATE TEMPORARY TABLE saved_$t LIKE $t;INSERT INTO saved_$t SELECT * FROM $t WHERE $($filters[$t]);CREATE TEMPORARY TABLE $t LIKE saved_$t;INSERT INTO $t SELECT * FROM saved_$t;`n"}
 $setup+="DELETE FROM conditions WHERE Comment LIKE 'Lights Wrath pickup - %';"
 function Snapshot([string]$label){($filters.Keys|ForEach-Object {"SELECT '$label-$_',a.* FROM $_ a;"}) -join "`n"}
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 function Query([string]$sql,[switch]$Expected){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$db[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names --force 2> "$out/sql-stderr.txt");$code=$LASTEXITCODE}finally{$env:MYSQL_PWD=$previous};$err=Get-Content "$out/sql-stderr.txt" -Raw;if($Expected){if($err -notmatch '1644 \(45000\)' -or @($err -split "`n"|Where-Object {$_ -match '^ERROR '}).Count -ne 1){throw 'Expected precise refusal missing'}}elseif($code -or $err){throw 'Lights Wrath SQL query failed'};$rows}
 function Rows($rows,[string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)}|Sort-Object -CaseSensitive)}
 function Signature($rows,[string]$label){(Rows $rows $label) -join "`n"}
 $query="$setup`n$migration`n$(Snapshot 'first')`n$migration`n$(Snapshot 'second')"
 foreach($t in @('gameobject_template','gameobject_loot_template','smart_scripts')){$query+="SELECT 'expected-$t',a.* FROM saved_$t a;"}
 foreach($scope in @(@('loot','SourceTypeOrReferenceId=4 AND SourceGroup=248572'),@('root','SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=248572 AND SourceId=1'),@('cast','SourceTypeOrReferenceId=17 AND SourceEntry=208011'))){$query+="SELECT '$($scope[0])',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $($scope[1]) ORDER BY ElseGroup,ConditionTypeOrReference;"}
 $rows=Query $query
 foreach($t in $filters.Keys){if((Signature $rows "first-$t") -cne (Signature $rows "second-$t")){throw 'Non-idempotent conditions'}}
 foreach($t in @('gameobject_template','gameobject_loot_template','smart_scripts')){if((Signature $rows "first-$t") -cne (Signature $rows "expected-$t")){throw 'Native graph/loot/template changed'}}
 foreach($kind in @('loot','root','cast')){[IO.File]::WriteAllLines("$out/$kind.tsv",[string[]](Rows $rows $kind));$count=if($kind -eq 'loot'){14}else{28};if((Rows $rows $kind).Count -ne $count){throw 'Native predicate row count differs'}}
 foreach($change in @("UPDATE gameobject_template SET ScriptName='custom';",'UPDATE gameobject_template SET Data0=43;','UPDATE gameobject_loot_template SET Item=1;','UPDATE smart_scripts SET link=0 WHERE id=0;',"INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1,Comment) VALUES(4,248572,128868,15,1,NULL);","INSERT INTO conditions(SourceTypeOrReferenceId,SourceEntry,ConditionTypeOrReference,ConditionValue1,Comment) VALUES(17,208011,15,1,'custom');",'DELETE FROM smart_scripts WHERE id=1;')){
  $refused=Query "$setup`n$change`n$(Snapshot 'before')`n$migration`n$(Snapshot 'after')" -Expected
  foreach($t in $filters.Keys){if((Signature $refused "before-$t") -cne (Signature $refused "after-$t")){throw 'Refused migration changed TEMP data'}}
 }
 $s=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
 $cases=(Section $s '        case CONDITION_ITEM:' '        case CONDITION_ITEM_EQUIPPED:')+(Section $s '        case CONDITION_CLASS:' '        case CONDITION_RACE:')+(Section $s '        case CONDITION_QUESTREWARDED:' '        case CONDITION_QUEST_NONE:')+(Section $s '        case CONDITION_MAPID:' '        case CONDITION_AREAID:')+(Section $s '        case CONDITION_SCENARION_STEP:' '        case CONDITION_REALM_ACHIEVEMENT:')+(Section $s '        case CONDITION_ALIVE:' '        case CONDITION_HP_VAL:')
 [IO.File]::WriteAllText("$out/Conditions.inc",$cases)
 [IO.File]::WriteAllText("$out/ElseGroups.inc",(Section $s 'bool ConditionMgr::IsObjectMeetToConditionList(' 'bool ConditionMgr::IsObjectMeetToConditions(WorldObject* object,'))
 $s=Get-Content "$repo/src/server/game/Loot/LootMgr.cpp" -Raw
 [IO.File]::WriteAllText("$out/LootCondition.inc",(Section $s 'bool LootItem::AllowedForPlayer(Player const* player) const' 'void LootItem::AddAllowedLooter('))
 $recheck=Section $s '    if (item && !AllowedForPlayer(player,' 'uint32 Loot::GetMaxSlotInLootFor('
 [IO.File]::WriteAllText("$out/SlotRecheck.inc",$recheck.Substring(0,$recheck.LastIndexOf('}')))
 $s=Get-Content "$repo/src/server/game/Entities/GameObject/GameObjectData.cpp" -Raw
 [IO.File]::WriteAllText("$out/GetLootId.inc",(Section $s 'uint32 GameObjectTemplate::GetLootId() const' 'uint32 GameObjectTemplate::GetGossipMenuId() const'))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncd /d `"$out`"`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/LightsWrathConditionsTest.cpp`" /Fe:test.exe /Fo:test.obj`nif errorlevel 1 exit /b 1`ntest.exe loot.tsv root.tsv cast.tsv`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native conditions regression failed'}
 $s=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
 [IO.File]::WriteAllText("$out/CastConditions.inc",(Section $s '    ConditionSourceInfo condInfo = ConditionSourceInfo(m_caster);' "    // Don't check explicit target for passive spells"))
 $s=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
 [IO.File]::WriteAllText("$out/NotGroupedConditions.inc",(Section $s 'ConditionList ConditionMgr::GetConditionsForNotGroupedEntry(' 'ConditionList ConditionMgr::GetConditionsForSpellClickEvent('))
 $s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 $first=$s.IndexOf('tempLastInvoker->AddDelayedEvent(200,');$last=$s.IndexOf('});',$first)
 if($first -lt 0 -or $last -lt $first){throw 'Native action85 deferred payload missing'}
 [IO.File]::WriteAllText("$out/DeferredInvoker.inc",$s.Substring($first,$last+3-$first))
 [IO.File]::WriteAllText("$out/cast.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncd /d `"$out`"`ncl /nologo /EHsc /std:c++17 /W4 $asan /DPICKUP_SPELL_ID=208011 /I`"$out`" `"$PSScriptRoot/TitanstrikePickupCastTest.cpp`" /Fe:cast.exe /Fo:cast.obj`nif errorlevel 1 exit /b 1`ncast.exe`nexit /b %errorlevel%`n")
 & "$out/cast.cmd";if($LASTEXITCODE){throw 'Native delayed cast consumer failed'}
 if($VerifyNativeCatalog){
  $prior=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile LightsWrathCatalog.cpp -ProbeName LightsWrathCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$prior}
  $native=@(Get-Content "$out/native.tsv")
  foreach($line in @("LFG`t1228`t1065`t1583`t12","LFG`t1241`t1101`t1583`t12","CRITERION`t1065`t5`t47119`t29657`t92`t49811","CRITERION`t1065`t6`t47172`t29682`t92`t49875","CRITERION`t1065`t6`t47214`t29707`t36`t128868","EFFECT`t208011`t2`t249`t0`t128868`t0`t1`t0","EFFECT`t208011`t3`t90`t0`t105109`t0`t1`t0")){if($native -notcontains $line){throw 'Native shared Nexus/final-step/claim contract changed'}}
 }
 if($VerifyLive){
  $sha=[Security.Cryptography.SHA1]::Create();try{$hash=[BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes(([IO.File]::ReadAllText((Resolve-Path $MigrationPath))).Replace("`r`n","`n")))).Replace('-','')}finally{$sha.Dispose()}
  $liveQuery="SET SESSION TRANSACTION READ ONLY;SELECT 'update',hash,state FROM updates WHERE name='2026_10_27_02_lights_wrath_pickup_admission.sql';"
  foreach($scope in @(@('loot','SourceTypeOrReferenceId=4 AND SourceGroup=248572'),@('root','SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=248572 AND SourceId=1'),@('cast','SourceTypeOrReferenceId=17 AND SourceEntry=208011'))){$liveQuery+="SELECT '$($scope[0])',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $($scope[1]);"}
  $live=Query $liveQuery;if($live -notcontains "update`t$hash`tRELEASED"){throw 'Live native updater hash differs'}
  foreach($kind in @('loot','root','cast')){if((Signature $rows $kind) -cne (Signature $live $kind)){throw 'Live rules differ from tested TEMP rows'}}
 }
 'PASS native source4/cached-slot and root/cast predicates, repeated TEMP migration/native data preservation/seven exact custom refusals; live character state unchanged.'
}
