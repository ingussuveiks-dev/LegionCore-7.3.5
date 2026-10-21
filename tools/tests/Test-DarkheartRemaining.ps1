param([switch]$VerifyReleaseBinary,[switch]$VerifyLiveDatabase)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/darkheart-remaining"
New-Item -ItemType Directory -Force $out | Out-Null
function Extract([string]$source,[string]$start,[string]$end,[string]$name){
    $a=$source.IndexOf($start);if($a -lt 0){throw "Missing production start: $start"}
    $b=$source.IndexOf($end,$a);if($b -le $a){throw "Missing production end: $end"}
    [IO.File]::WriteAllText("$out/$name.inc",$source.Substring($a,$b-$a))
}
$trash=Get-Content "$repo/src/server/scripts/Legion/DarkHeartThicket/darkheart_thicket_trash.cpp" -Raw
Extract $trash 'struct npc_darkheart_bloodtainted_burster' 'void AddSC_darkheart_thicket_trash()' 'Burster'
$glaidalis=Get-Content "$repo/src/server/scripts/Legion/DarkHeartThicket/boss_arch_druid_glaidalis.cpp" -Raw
Extract $glaidalis 'class spell_nightmare_toxin' '//225484 trash' 'Toxin'
$instance=Get-Content "$repo/src/server/scripts/Legion/DarkHeartThicket/instance_darkheart_thicket.cpp" -Raw
Extract $instance '        void OnCreatureCreate(' '        void OnGameObjectCreate(' 'DwellerLifecycle'
Extract $instance '        uint32 GetData(' '        WorldLocation* GetClosestGraveYard(' 'DwellerGetData'
Copy-Item "$repo/src/server/scripts/Legion/DarkHeartThicket/darkheart_thicket.h" "$out/DarkheartHeader.inc"
$oak=Get-Content "$repo/src/server/scripts/Legion/DarkHeartThicket/boss_oakheart.cpp" -Raw
Extract $oak '        void Reset() override' '        void EnterCombat(' 'OakAwakening'
Extract $oak '        void MoveInLineOfSight(' '        void UpdateAI(' 'OakLos'
$smart=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
Extract $smart '        case SMART_EVENT_HEALTH_PCT:' '        case SMART_EVENT_TARGET_HEALTH_PCT:' 'SmartHealth'
Extract $smart '        case SMART_EVENT_RANGE:' '        case SMART_EVENT_VICTIM_CASTING:' 'SmartRange'
Extract $smart '        case SMART_TARGET_HOSTILE_RANDOM_NOT_TOP:' '        case SMART_TARGET_HOSTILE_RANDOM_NOT_TOP_PLAYER:' 'SmartRandom'
$spell=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
Extract $spell '    // check death state' '    if (!strict && m_spellInfo->talentId)' 'NativeCasterDeath'
$spell=$spell.Substring($spell.IndexOf('void Spell::SelectImplicitCasterDestTargets('))
Extract $spell '        case TARGET_DEST_CASTER_RADIUS:' '        default:' 'NativeBackDestination'
$effect=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
$a=$effect.IndexOf('void Spell::EffectChargeDest(');$b=$effect.IndexOf('void Spell::EffectKnockBack(',$a)
$method=$effect.Substring($a,$b-$a);$body=$method.Substring($method.IndexOf('{')+1);$body=$body.Substring(0,$body.LastIndexOf('}'))
[IO.File]::WriteAllText("$out/NativeCharge.inc",$body)

$db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
function Query([string]$sql){
    $prior=$env:MYSQL_PWD
    try{$env:MYSQL_PWD=$db[3];$result=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Scoped Darkheart remaining SQL failed'};return $result}finally{$env:MYSQL_PWD=$prior}
}
$sql=''
$scopes=[ordered]@{
 creature_template='entry IN(100532,113398,95769)'
 smart_scripts='source_type=0 AND entryorguid IN(99358,95779,100527,113398,101991,95769)'
 conditions='SourceEntry IN(204852,204243)'
}
foreach($table in $scopes.Keys){$sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table WHERE $($scopes[$table]); CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"}
$sql+=@'
UPDATE creature_template SET AIName='',ScriptName='' WHERE entry IN(100532,113398);
DELETE FROM smart_scripts WHERE source_type=0 AND ((entryorguid=99358 AND id IN(2,3)) OR entryorguid=113398 OR (entryorguid=101991 AND event_type=6));
UPDATE smart_scripts SET event_param1=60,event_param2=65 WHERE entryorguid=95779 AND action_param1=200580;
UPDATE smart_scripts SET event_param1=15,event_param2=95 WHERE entryorguid=95779 AND action_param1=200589;
UPDATE smart_scripts SET event_param1=49,event_param2=51,target_type=6 WHERE entryorguid=100527 AND action_param1=201399;
INSERT INTO smart_scripts (entryorguid,source_type,id,event_type,action_type,action_param1,target_type,comment) VALUES (101991,0,1,6,11,204852,2,'Broken death action');
DELETE FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=204852;
'@
$migration=(Get-Content "$repo/sql/updates/world/2026_10_21_09_darkheart_remaining_trash.sql" -Raw)+"`n"+(Get-Content "$repo/sql/updates/world/2026_10_21_10_oakheart_awakening.sql" -Raw)
$snapshot=@'
SELECT 'CT',entry,AIName,ScriptName FROM creature_template ORDER BY entry;
SELECT 'SAI',entryorguid,id,link,event_type,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,target_type,Difficulties FROM smart_scripts ORDER BY entryorguid,id;
SELECT 'COND',SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1,ConditionValue2 FROM conditions ORDER BY SourceEntry,SourceGroup,ConditionTypeOrReference,ConditionValue1,ConditionValue2;
'@
$sql+="`nSELECT 'ITERATION',1;`n$migration`n$snapshot`nSELECT 'ITERATION',2;`n$migration`n$snapshot`n"
$sql+=@'
SELECT 'ROW',entryorguid,event_type,event_param1,event_param2,target_type FROM smart_scripts WHERE source_type=0 AND ((entryorguid=99358 AND id=2) OR (entryorguid=95779 AND event_type=2) OR (entryorguid=100527 AND event_type=2)) ORDER BY entryorguid,id;
SELECT 'ASSERT', 'fury_no_dispersion',COUNT(*)=0 FROM smart_scripts WHERE entryorguid=113398 AND event_type=6;
SELECT 'ASSERT', 'burster_script_binding',COUNT(*)=1 FROM creature_template WHERE entry=100532 AND AIName='' AND ScriptName='npc_darkheart_bloodtainted_burster';
SELECT 'ASSERT', 'fury_ai_binding',COUNT(*)=1 FROM creature_template WHERE entry=113398 AND AIName='SmartAI' AND ScriptName='';
SELECT 'ASSERT', 'fury_abilities',COUNT(*)=2 FROM smart_scripts WHERE entryorguid=113398 AND action_param1 IN(201226,201272);
SELECT 'ASSERT', 'dweller_eye',COUNT(*)=1 FROM smart_scripts WHERE entryorguid=101991 AND action_param1=204243;
SELECT 'ASSERT', 'dweller_death_removed',COUNT(*)=0 FROM smart_scripts WHERE entryorguid=101991 AND event_type=6;
SELECT 'ASSERT', 'health_oneshot',COUNT(*)=2 FROM smart_scripts WHERE event_flags=1 AND event_type=2 AND ((entryorguid=95779 AND action_param1=200580) OR (entryorguid=100527 AND action_param1=201399));
SELECT 'ASSERT', 'dryad_chain',COUNT(*)=2 FROM smart_scripts WHERE entryorguid=99358 AND ((id=2 AND link=3 AND action_type=66 AND target_type=2) OR (id=3 AND event_type=61 AND action_type=11 AND action_param1=198792 AND target_type=1));
SELECT 'ASSERT','unrelated_template',COUNT(*)=1 FROM creature_template c JOIN saved_creature_template s ON c.entry=s.entry WHERE c.entry=95769 AND c.AIName=s.AIName AND c.ScriptName=s.ScriptName;
SELECT 'ASSERT','unrelated_sai',COUNT(*)=(SELECT COUNT(*) FROM saved_smart_scripts WHERE entryorguid=95769) FROM smart_scripts c JOIN saved_smart_scripts s ON c.entryorguid=s.entryorguid AND c.id=s.id AND c.source_type=s.source_type WHERE c.entryorguid=95769 AND c.action_param1=s.action_param1 AND c.event_flags=s.event_flags;
'@
$rows=@(Query $sql);$rows | Set-Content "$out/shadow.tsv"
$first=[Array]::IndexOf($rows,"ITERATION`t1");$second=[Array]::IndexOf($rows,"ITERATION`t2");$tail=@($rows|Where-Object {$_ -match '^ROW\t|^ASSERT\t'})
$one=@($rows[($first+1)..($second-1)]);$two=@($rows[($second+1)..($rows.Count-$tail.Count-1)])
if(($one -join "`n") -ne ($two -join "`n")){throw 'Migration repeat changed the Darkheart snapshot'}
$asserts=@($rows | Where-Object {$_ -match '^ASSERT\t'});if($asserts.Count -ne 10 -or @($asserts | Where-Object {$_ -notmatch '\t1$'}).Count){throw 'Shadow migration contract failed'}
if($VerifyLiveDatabase){
    $bindings=@(Query "SELECT entry,AIName,ScriptName FROM creature_template WHERE entry IN(100532,113398) ORDER BY entry;")
    if(($bindings -join "`n") -cne "100532`t`tnpc_darkheart_bloodtainted_burster`n113398`tSmartAI`t"){
        throw 'Live Darkheart script bindings differ from migration 2026_10_21_09; shadow replay alone does not verify deployment'
    }
    $bindings | Set-Content "$out/live-bindings.tsv"
    'PASS: live Burster script and regenerated Fury SmartAI bindings match the migration.'
}
@($rows|Where-Object {$_.StartsWith("ROW`t")}|ForEach-Object {$_.Substring(4)}) | Set-Content "$out/smart-rows.tsv"
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($fixture in @('DarkheartRemainingTrashTest','DarkheartSmartTrashTest','OakheartAwakeningTest')){
    $extra=if($fixture -eq 'DarkheartSmartTrashTest'){" `"$out/smart-rows.tsv`""}else{''}
    $cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/$fixture.cpp`" /Fe`"$out/$fixture.exe`" /Fo`"$out/$fixture.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/$fixture.exe`"$extra`r`n"
    [IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd";if($LASTEXITCODE){throw "Production fixture failed: $fixture"}
}
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile DarkheartSpellCatalog.cpp -ProbeName DarkheartSpellCatalog -OutputFile "$out/native.tsv"
if($LASTEXITCODE){throw 'Native spell reader failed'}
$native=Get-Content "$out/native.tsv"
foreach($pattern in @('^E\t198792\t0\t0\t149\t0\t0\t0\t100\t0\t48\t86\t','^E\t225562\t0\t0\t28\t0\t113398\t','^M\t225562\t0\t14\t21\t-1\t1\t3000$','^E\t201298\t0\t0\t9\t','^M\t201298\t0\t14\t0\t0\t5\t3000$','^E\t165950\t0\t0\t6\t12\t','^E\t165953\t0\t0\t164\t0\t0\t0\t0\t165950\t','^E\t204852\t0\t0\t3\t0\t0\t0\t0\t0\t38\t')){
    if(!@($native|Where-Object {$_ -match $pattern}).Count){throw "Native 7.3.5 contract changed: $pattern"}
}
if($native -notcontains "R`t198792`t0`t0`t15`t0`t15"){throw 'Prancing Escape native distance changed'}
$hotfix=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$prior=$env:MYSQL_PWD
try{
    $env:MYSQL_PWD=$hotfix[3]
    $overrides=@("SELECT COUNT(*) FROM spell_effect WHERE SpellID IN(198792,225562,201298,165950,165953,204852,200684,200686); SELECT COUNT(*) FROM spell_misc WHERE SpellID IN(198792,225562,201298,165950,165953,204852,200684,200686); SELECT COUNT(*) FROM spell_target_restrictions WHERE SpellID IN(198792,225562,201298,165950,165953,204852,200684,200686);" | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($hotfix[0]) --port=$($hotfix[1]) --user=$($hotfix[2]) --database=$($hotfix[4]) --batch --raw --skip-column-names)
    if($LASTEXITCODE -or ($overrides -join ',') -ne '0,0,0'){throw 'Review changed live spell hotfix contracts'}
}finally{$env:MYSQL_PWD=$prior}
if($VerifyReleaseBinary){
    & 'C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' "$PSScriptRoot/verify_darkheart_remaining_release.py" "$repo/build-extractors/bin/Release"
    if($LASTEXITCODE){throw 'Actual deployed Darkheart follow-up handlers missing'}
}
'PASS: remaining Darkheart SQL is repeatable and scoped; actual handlers and native 7.3.5 spell contracts passed. Client traversal/visuals remain separate.'
