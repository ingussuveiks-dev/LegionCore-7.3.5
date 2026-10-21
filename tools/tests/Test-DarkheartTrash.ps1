param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/darkheart-trash"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$source=Get-Content "$repo/src/server/scripts/Legion/DarkHeartThicket/darkheart_thicket_trash.cpp" -Raw
$start=$source.IndexOf('namespace DarkheartTrash');$end=$source.IndexOf('// 100532.')
if($start -lt 0 -or $end -le $start){throw 'Missing production trash handlers'}
[IO.File]::WriteAllText("$out/Trash.inc",$source.Substring($start,$end-$start))
$glaidalis=Get-Content "$repo/src/server/scripts/Legion/DarkHeartThicket/boss_arch_druid_glaidalis.cpp" -Raw
$trail=$glaidalis.IndexOf('        if (trailTimer <= diff)');$trailEnd=$glaidalis.IndexOf('        if (fixateTimer)',$trail)
if($trail -lt 0 -or $trailEnd -le $trail){throw 'Missing native abomination trail callback'}
[IO.File]::WriteAllText("$out/Trail.inc",$glaidalis.Substring($trail,$trailEnd-$trail))
$area=Get-Content "$repo/src/server/game/Entities/AreaTrigger/AreaTrigger.cpp" -Raw
$area=$area.Substring($area.IndexOf('void AreaTrigger::DoAction(Unit* unit, ActionInfo& action)'))
foreach($section in @(
 @('    if (action.action->targetFlags & AT_TARGET_FLAG_VALIDATTACK)','    if (action.action->targetFlags & AT_TARGET_FLAG_NOT_IN_LOS)','AreaAttack'),
 @('    if (action.action->targetFlags & AT_TARGET_FLAG_PLAYER)','    if (action.action->targetFlags & AT_TARGET_FLAG_NOT_PET)','AreaPlayer'),
 @('    // should cast on self.','    if (action.action->targetFlags & AT_TARGET_FLAG_CASTER_AURA_TARGET)','AreaSelfCaster'),
 @('        case AT_ACTION_TYPE_CAST_SPELL:','        case AT_ACTION_TYPE_REMOVE_AURA:','AreaCast'),
 @('        case AT_ACTION_TYPE_REMOVE_AURA_BY_CASTER:','        case AT_ACTION_TYPE_CAST_SPELL_NOT_TRIGGER:','AreaRemove')
)){$a=$area.IndexOf($section[0]);$b=$area.IndexOf($section[1],$a);if($a -lt 0 -or $b -le $a){throw 'Missing native area action branch'};[IO.File]::WriteAllText("$out/$($section[2]).inc",$area.Substring($a,$b-$a))}
$dummy=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
foreach($section in @(
 @('            int32 spell_trigger = damage;','            if(triggerCaster && !triggerCaster->IsInWorld())','DummySpellId'),
 @('                case DUMMY_TRIGGER_CAST_IGNORE_GCD:','                case DUMMY_TRIGGER_MOVE_AURA:','DummyMissile')
)){$a=$dummy.IndexOf($section[0]);$b=$dummy.IndexOf($section[1],$a);if($a -lt 0 -or $b -le $a){throw 'Missing native dummy trigger branch'};[IO.File]::WriteAllText("$out/$($section[2]).inc",$dummy.Substring($a,$b-$a))}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/DarkheartTrashTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Extracted Darkheart trash lifecycle regression failed'}

# Replay on connection-local shadow tables, never on live character/content rows.
$scopes=[ordered]@{
 creature_template='entry IN(95766,99358,99359,99360,100960,101958,100539,100529,108460)'
 smart_scripts='source_type=0 AND entryorguid IN(95766,99358,99359,99360,100960,101958,100539,100529,100531,108460)'
 spell_dummy_trigger='spell_id IN(198400,198799)'
 spell_script_names='spell_id IN(204243,204970,200684,212464,200289)'
 areatrigger_actions='entry IN(5445,5451,5486,5670,5676,5714,6078,5529,6084,8722,5631,8082)'
 areatrigger_template='entry IN(5445,5529,6084,8722)'
}
$sql=''
foreach($table in $scopes.Keys){$sql+="CREATE TEMPORARY TABLE dht_saved_$table LIKE $table; INSERT INTO dht_saved_$table SELECT * FROM $table WHERE $($scopes[$table]); CREATE TEMPORARY TABLE $table LIKE dht_saved_$table; INSERT INTO $table SELECT * FROM dht_saved_$table;`n"}
# Recreate the broken baseline even after the live server has been repaired.
$sql+=@'
UPDATE creature_template SET AIName='',ScriptName='' WHERE entry<>108460;
DELETE FROM smart_scripts WHERE source_type=0 AND entryorguid IN(95766,99358,99360,100539,100529);
INSERT INTO smart_scripts (entryorguid,source_type,id,event_type,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,target_type,comment)
VALUES (99360,0,0,0,21000,22000,21000,22000,11,201123,2,'Baseline Blossom without warning');
DELETE FROM spell_dummy_trigger WHERE spell_id=198799;
DELETE FROM spell_script_names WHERE ScriptName IN('spell_darkheart_tormenting_eye','spell_darkheart_noxious_spit','spell_darkheart_mythic_nightfall','spell_darkheart_growing_paranoia');
DELETE FROM areatrigger_actions WHERE entry IN(5451,5670,5676,5714,6078,5529,6084,8722,8082);
DELETE FROM areatrigger_template WHERE entry IN(5529,6084,8722);
INSERT INTO areatrigger_template (entry,spellId,customEntry,VisualID,Radius,RadiusTarget,windSpeed,windType,comment) VALUES
 (5529,199345,10239,199345,75,75,-10,1,'Broken duplicate'),(5529,199348,10771,51790,50,50,-7,1,'Native wind'),
 (6084,199345,10239,199345,75,75,-10,1,'Broken duplicate'),(6084,199348,10771,51790,50,50,-7,1,'Native wind'),
 (8722,199345,10239,199345,75,75,-10,1,'Broken duplicate'),(8722,199348,10771,51790,50,50,-7,1,'Native wind');
'@
$migration=Get-Content "$repo/sql/updates/world/2026_10_21_06_darkheart_trash_hazards.sql" -Raw
$migration+="`n"+(Get-Content "$repo/sql/updates/world/2026_10_21_07_darkheart_boss_hazards.sql" -Raw)
$snapshot=@'
SELECT 'CT',entry,AIName,ScriptName FROM creature_template ORDER BY entry;
SELECT 'SAI',entryorguid,id,link,event_type,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,target_type,Difficulties FROM smart_scripts ORDER BY entryorguid,source_type,id,link;
SELECT 'DUMMY',spell_id,spell_trigger,`option`,effectmask,handlemask FROM spell_dummy_trigger ORDER BY spell_id,spell_trigger,`option`,aura;
SELECT 'SCRIPT',spell_id,ScriptName FROM spell_script_names ORDER BY spell_id,ScriptName;
SELECT 'AT',entry,customEntry,id,moment,actionType,targetFlags,spellId FROM areatrigger_actions ORDER BY entry,customEntry,id;
SELECT 'TEMPLATE',entry,spellId,customEntry,VisualID,Radius,RadiusTarget,windSpeed,windType FROM areatrigger_template ORDER BY entry,spellId,customEntry;
'@
$sql+="$migration`nSELECT 'FIRST';`n$snapshot`n$migration`nSELECT 'SECOND';`n$snapshot`nSELECT 'END';`n"
$sql+=@'
SELECT 'UNCHANGED_CT',COUNT(*) FROM creature_template a JOIN dht_saved_creature_template b USING(entry) WHERE a.entry=108460 AND a.AIName=b.AIName AND a.ScriptName=b.ScriptName;
SELECT 'UNCHANGED_SAI',COUNT(*) FROM smart_scripts a JOIN dht_saved_smart_scripts b USING(entryorguid,source_type,id,link) WHERE a.entryorguid=108460 AND a.action_param1<>b.action_param1;
SELECT 'UNCHANGED_AT',COUNT(*) FROM areatrigger_actions a JOIN dht_saved_areatrigger_actions b USING(entry,customEntry,id) WHERE a.entry=5445 AND (a.spellId<>b.spellId OR a.actionType<>b.actionType OR a.targetFlags<>b.targetFlags);
'@
$config=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$config[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($config[0]) --port=$($config[1]) --user=$($config[2]) --database=$($config[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Darkheart shadow-table replay failed'}}finally{$env:MYSQL_PWD=$previous}
$a=[Array]::IndexOf($rows,'FIRST');$b=[Array]::IndexOf($rows,'SECOND');$c=[Array]::IndexOf($rows,'END')
if(($rows[($a+1)..($b-1)] -join "`n") -ne ($rows[($b+1)..($c-1)] -join "`n")){throw 'Darkheart migration is not idempotent'}
foreach($expected in @(
 "CT`t99359`t`tnpc_darkheart_rotheart_keeper","CT`t99360`t`tnpc_darkheart_vilethorn_blossom",
 "CT`t100960`t`tnpc_darkheart_vile_mushroom","CT`t101958`t`tnpc_darkheart_root_burst",
 "DUMMY`t198799`t0`t11`t1`t9","SCRIPT`t204243`tspell_darkheart_tormenting_eye","SCRIPT`t204970`tspell_darkheart_noxious_spit",
 "AT`t5670`t10377`t0`t1`t0`t10`t200771","AT`t5714`t10424`t0`t1`t0`t10`t201227",
 "AT`t5676`t10383`t0`t1`t0`t10`t200822","AT`t5676`t10383`t1`t42`t13`t10`t200822",
 "AT`t5451`t10154`t0`t1`t0`t10`t198408","AT`t5451`t10154`t1`t42`t13`t10`t198408",
 "AT`t6078`t10765`t0`t1`t0`t10`t204976","AT`t6078`t10765`t1`t42`t13`t10`t204976",
 "UNCHANGED_CT`t1","UNCHANGED_SAI`t0","UNCHANGED_AT`t0"
 ,"TEMPLATE`t5529`t199345`t10771`t51790`t50`t50`t-7`t1"
 ,"TEMPLATE`t6084`t199345`t10771`t51790`t50`t50`t-7`t1"
 ,"TEMPLATE`t8722`t199345`t10771`t51790`t50`t50`t-7`t1"
 ,"SCRIPT`t212464`tspell_darkheart_mythic_nightfall","SCRIPT`t200289`tspell_darkheart_growing_paranoia"
 ,"AT`t8082`t10343`t0`t1`t0`t49160`t200329"
)){if($rows -notcontains $expected){throw "Darkheart content mismatch: $expected"}}
$windRows=@($rows[($a+1)..($b-1)] | Where-Object {$_ -match '^TEMPLATE\t(5529|6084|8722)\t'})
if($windRows.Count -ne 3){throw 'Conflicting Down Draft templates survived'}
[IO.File]::WriteAllLines("$out/actions.tsv",[string[]]@($rows[($a+1)..($b-1)] | Where-Object {$_ -like "AT`t*" -and $_ -notmatch '^AT\t(5631|8082)\t'}))
$areaCmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/DarkheartAreaActionsTest.cpp`" /Fe`"$out/area-test.exe`" /Fo`"$out/area-test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/area-test.exe`" `"$out/actions.tsv`"`r`n"
[IO.File]::WriteAllText("$out/area-run.cmd",$areaCmd);& "$out/area-run.cmd"
if($LASTEXITCODE){throw 'Native area action regression failed'}
$sai=@($rows[($a+1)..($b-1)] | Where-Object {$_ -like "SAI`t*"} | ForEach-Object {,@($_ -split "`t")})
foreach($id in @(95766,99358,100539,100529)){if(!($sai | Where-Object {[int]$_[1] -eq $id})){throw "Missing executable AI for $id"}}
$charge=$sai | Where-Object {$_[1] -eq '95766' -and $_[2] -eq '0'}
if($charge[3] -ne '1' -or $charge[10] -ne '66' -or $charge[13] -ne '125'){throw 'Charge must face a Player before the linked self cast'}
foreach($row in $sai | Where-Object {$_[1] -in @('95766','99358','100539','100529')}){if($row[5] -ne '0' -or $row[14] -ne ''){throw 'Outdoor/dungeon difficulty admission regression'}}

# Native C++ DB2 reader, not guessed fixed-width Python offsets.
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile DarkheartSpellCatalog.cpp -ProbeName DarkheartSpellCatalog -OutputFile "$out/catalog.tsv"
if($LASTEXITCODE){throw 'Native Darkheart spell catalog failed'}
$native=@(Get-Content "$out/catalog.tsv")
function NativeRow([string]$kind,[int]$id,[int]$difficulty=0,[int]$effect=-1){$prefix="$kind`t$id`t$difficulty`t";if($effect -ge 0){$prefix+="$effect`t"};$r=@($native | Where-Object {$_.StartsWith($prefix)});if($r.Count -ne 1){throw "Native row unresolved: $prefix"};return ,@($r[0] -split "`t")}
$root=NativeRow E 201123 0 0;if($root[10] -ne '25'){throw 'Root Burst no longer permits its native ANY-unit warning target'}
$rootAura=NativeRow A 201123;if(([int]$rootAura[6] -band 0x100) -ne 0){throw 'Root Burst player-only flag blocks warning stalker'}
$rootCast=NativeRow M 201123;if($rootCast[7] -ne '3000'){throw 'Root Burst warning/cast timing mismatch'}
$model=NativeRow V 198937;$visual=NativeRow V 201145
if($model[3] -ne '51690' -or $visual[3] -ne '52393'){throw 'Wrong native mushroom/root visual'}
$normal=NativeRow T 198799;$mythic=NativeRow T 198799 23
if($normal[3] -ne '1' -or $mythic[3] -ne '3'){throw 'Vile Mushroom target count regression'}
$missile=NativeRow E 220369 0 0;if($missile[9] -ne '198910'){throw 'Broken native Vile Mushroom missile'}
$summon=NativeRow E 198910 0 0;if($summon[4] -ne '28' -or $summon[6] -ne '100960'){throw 'Wrong mushroom summon'}
$warning=NativeRow E 201129 0 0;if($warning[6] -ne '101958'){throw 'Wrong Root Burst warning helper'}
$spit=NativeRow E 204970 0 0;if($spit[9] -ne '204972' -or $spit[13] -ne '1000'){throw 'Noxious Spit fallback changed native periodic contract'}
if($native -notcontains "S`t204972`tMISSING`t"){throw 'Review fallback: native trigger 204972 is now present'}
$hotfix=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$hotfixSql=@'
SELECT 'TRIGGER',COUNT(*) FROM spell WHERE ID=204972;
SELECT 'EFFECT',COUNT(*) FROM spell_effect WHERE SpellID IN(204972,204970,201123,198799,212464,199345,225497,225503);
SELECT 'MISC',COUNT(*) FROM spell_misc WHERE SpellID IN(204972,204970,201123,198799,212464,199345,225497,225503);
SELECT 'TARGET',COUNT(*) FROM spell_target_restrictions WHERE SpellID IN(198799,201123);
'@
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$hotfix[3];$hotfixRows=@($hotfixSql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($hotfix[0]) --port=$($hotfix[1]) --user=$($hotfix[2]) --database=$($hotfix[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Hotfix contract audit failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($kind in @('TRIGGER','EFFECT','MISC','TARGET')){if($hotfixRows -notcontains "$kind`t0"){throw "Review native contract: live $kind hotfix is present"}}
if($VerifyReleaseBinary){
 $python='C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
 & $python "$PSScriptRoot/verify_darkheart_trash_release.py" "$repo/build-extractors/bin/Release"
 if($LASTEXITCODE){throw 'Deployed Darkheart implementation check failed'}
}
'PASS: shadow SQL repeat/unrelated preservation, native 7.3.5 target/visual/cast/missile contracts and production AI fixtures. Full dungeon traversal and rendered visuals still require the client.'
