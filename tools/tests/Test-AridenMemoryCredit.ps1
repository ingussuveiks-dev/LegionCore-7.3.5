$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/ariden-memory-credit"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Extract([string]$s,[string]$start,[string]$end,[string]$name){
 $a=$s.IndexOf($start);$b=$s.IndexOf($end,$a)
 if($a -lt 0 -or $b -lt $a){throw "Missing production section $start"}
 [IO.File]::WriteAllText("$out/$name.inc",$s.Substring($a,$b-$a))
}
$s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
Extract $s '        case SMART_ACTION_CAST:' '        case SMART_ACTION_CAST_CUSTOM:' 'CastAction'
Extract $s '        case SMART_ACTION_INVOKER_CAST:' '        case SMART_ACTION_ADD_AURA:' 'InvokerCast'
Extract $s '        case SMART_TARGET_OWNER_OR_SUMMONER:' '        case SMART_TARGET_THREAT_LIST:' 'OwnerTarget'
$s=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
Extract $s 'Unit* Unit::GetAnyOwner() const' 'Unit* Unit::GetCharmer() const' 'AnyOwner'
$s=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
Extract $s 'void Spell::EffectKillCreditPersonal(' 'void Spell::EffectKillCredit(' 'PersonalCredit'
$s=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
Extract $s '        case CONDITION_NEAR_CREATURE:' '        case CONDITION_NEAR_GAMEOBJECT:' 'NearCondition'
$s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
Extract $s 'enum SmartCastFlags' '// one line in DB' 'CastFlags'
$migration=Get-Content "$repo/sql/updates/world/2026_10_20_02_ariden_memory_pet_credit.sql" -Raw
$sql=@"
CREATE TEMPORARY TABLE saved_smart_scripts LIKE smart_scripts;
INSERT INTO saved_smart_scripts SELECT * FROM smart_scripts WHERE entryorguid=100735 AND source_type=0;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart_scripts;
INSERT INTO smart_scripts SELECT * FROM saved_smart_scripts;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT 22,1,100735,0,0,29,0,100735,30,0,1,0,'','Old killer origin';
INSERT INTO conditions SELECT 22,2,100735,0,0,29,0,100735,30,0,1,0,'','Unrelated event condition';
UPDATE smart_scripts SET action_type=85,action_param2=0,target_type=1 WHERE id=0;
$migration
$migration
SELECT 'ROW',action_type,action_param2,target_type FROM smart_scripts WHERE entryorguid=100735 AND source_type=0 AND id=0 AND event_type=6 AND action_param1=198287;
SELECT 'COND',ConditionTarget FROM conditions WHERE SourceEntry=100735 AND SourceGroup=1 AND ConditionValue2=30 AND NegativeCondition=1;
SELECT 'unrelated-condition',COUNT(*) FROM conditions WHERE SourceEntry=100735 AND SourceGroup=2 AND ConditionTarget=0;
SELECT 'preserved',COUNT(*) FROM smart_scripts n JOIN saved_smart_scripts o USING(entryorguid,source_type,id) WHERE n.id=1 AND n.event_type=o.event_type AND n.action_type=o.action_type AND n.action_param1=o.action_param1 AND n.target_type=o.target_type;
SELECT 'optional',COUNT(*) FROM quest_objectives WHERE QuestID IN (40604,40835,40932) AND ObjectID=100736 AND (Flags & 4)=4;
SELECT 'journal',COUNT(*) FROM gameobject_questender WHERE id=245793 AND quest IN (40604,40835,40932);
SELECT 'visuals',COUNT(*) FROM spell_linked_spell WHERE spell_trigger=198287 AND spell_effect IN (200234,200235) AND type=1;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$c[3]
 $rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Ariden SQL replay failed'}
}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("ROW`t11`t18`t23","COND`t1","unrelated-condition`t1","preserved`t1","optional`t3","journal`t3","visuals`t2")){if($rows -notcontains $expected){throw "Missing SQL assertion: $expected"}}
$row=($rows | Where-Object {$_ -like "ROW`t*"}).Split("`t")
$condition=($rows | Where-Object {$_ -like "COND`t*"}).Split("`t")[1]
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/AridenMemoryCreditTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" $($row[1]) $($row[2]) $($row[3]) $condition`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Ariden native credit regression failed'}
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile AridenCampCatalog.cpp -ProbeName AridenCampCatalog -OutputFile "$out/native.tsv"
$native=Get-Content "$out/native.tsv"
foreach($expected in @('effect 198276 0 28 100735 64 46 0','effect 198278 0 28 100735 64 46 0','effect 198287 0 134 100732 0 1 0')){if($native -notcontains $expected){throw "Native DB2 mismatch: $expected"}}
'PASS: repeated SQL migration, combat row/linked conversations preserved, shared quest hand-ins and optional objectives, native summon and personal-credit spell catalog'
