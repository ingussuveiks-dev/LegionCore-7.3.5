param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/titanstrike-runes"
New-Item -ItemType Directory -Force $out | Out-Null
$source=Get-Content "$repo/src/server/scripts/Legion/titanstrike_runes.cpp" -Raw
$a=$source.IndexOf('struct npc_titanstrike_prustaga');$b=$source.IndexOf('void AddSC_titanstrike_runes()',$a)
if($a -lt 0 -or $b -lt $a){throw 'Production rune actors missing'}
[IO.File]::WriteAllText("$out/TitanstrikeActors.inc",$source.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" /I`"$repo/src/server/scripts/Legion`" `"$PSScriptRoot/TitanstrikeRunesTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Production Titanstrike actor regression failed'}
$migration=Get-Content "$repo/sql/updates/world/2026_10_23_02_titanstrike_rune_waves.sql" -Raw
$snapshot=@'
SELECT 'sai',entryorguid,source_type,id,link,event_type,event_param1,event_param2,action_type,action_param1,action_param2,target_type,target_param1,target_param2 FROM smart_scripts ORDER BY entryorguid,source_type,id;
SELECT 'condition',SourceEntry,ConditionTarget FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=3 AND SourceEntry IN (106335,106341,106344) ORDER BY SourceEntry;
SELECT 'script',entry,AIName,ScriptName FROM creature_template ORDER BY entry;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_sai LIKE smart_scripts;
INSERT INTO saved_sai SELECT * FROM smart_scripts WHERE entryorguid IN (104949,10494900,10494901,104904,106341);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_sai;
INSERT INTO smart_scripts SELECT * FROM saved_sai;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry IN (106335,106341,106344);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
UPDATE conditions SET ConditionTarget=0;
CREATE TEMPORARY TABLE saved_creature LIKE creature_template;
INSERT INTO saved_creature SELECT * FROM creature_template WHERE entry IN (104949,104904);
CREATE TEMPORARY TABLE creature_template LIKE saved_creature;
INSERT INTO creature_template SELECT * FROM saved_creature;
UPDATE creature_template SET ScriptName='';
$migration
SELECT 'FIRST';
$snapshot
$migration
SELECT 'SECOND';
$snapshot
SELECT 'END';
SELECT 'first-delay',SUM(event_param1) FROM smart_scripts WHERE entryorguid=104949 AND source_type=9;
SELECT 'second-delay',SUM(event_param1) FROM smart_scripts WHERE entryorguid=10494900 AND source_type=9;
SELECT 'third-delay',SUM(event_param1) FROM smart_scripts WHERE entryorguid=10494901 AND source_type=9;
SELECT 'link-missing',COUNT(*) FROM smart_scripts a LEFT JOIN smart_scripts b ON b.entryorguid=a.entryorguid AND b.source_type=a.source_type AND b.id=a.link AND b.event_type=61 WHERE a.link<>0 AND b.id IS NULL;
UPDATE creature_template SET ScriptName='custom' WHERE entry=104949;
UPDATE smart_scripts SET action_type=12,action_param1=106335,target_type=8 WHERE entryorguid=104949 AND source_type=9 AND id=1;
$migration
SELECT 'custom',COUNT(*) FROM creature_template WHERE entry=104949 AND ScriptName='custom';
SELECT 'custom-data',COUNT(*) FROM smart_scripts WHERE entryorguid=104949 AND source_type=9 AND id=1 AND action_type=12 AND action_param1=106335;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Rune SQL fixture failed'}}finally{$env:MYSQL_PWD=$previous}
$a=[Array]::IndexOf($rows,'FIRST');$b=[Array]::IndexOf($rows,'SECOND');$d=[Array]::IndexOf($rows,'END')
$first=$rows[($a+1)..($b-1)];$second=$rows[($b+1)..($d-1)]
if(($first -join '|') -ne ($second -join '|')){throw 'Rune migration is not idempotent'}
foreach($expected in @("first-delay`t11000","second-delay`t9000","third-delay`t8000","custom`t1","custom-data`t1","link-missing`t0")){if($rows -notcontains $expected){throw "Missing rune result $expected"}}
foreach($expected in @("script`t104904`tSmartAI`tnpc_titanstrike_grif","script`t104949`tSmartAI`tnpc_titanstrike_prustaga","condition`t106335`t1","condition`t106341`t1","condition`t106344`t1")){if($first -notcontains $expected){throw "Missing rune actor/condition $expected"}}
$sai=@{}
foreach($line in $first){$f=$line.Split("`t");if($f[0] -eq 'sai'){$sai["$($f[1])/$($f[2])/$($f[3])"]=$f}}
# Traverse the actual migrated final chain. The optional cast must come last.
$id=12;$path=@();$actions=@()
while($id){if($path -contains $id){throw 'Cyclic final rune LINK'};$path+=$id;$row=$sai["104949/0/$id"];if(!$row){throw "Missing final row $id"};$actions+=[int]$row[8];$id=[int]$row[4]}
if(($path -join ',') -ne '12,14,15,16,13' -or ($actions -join ',') -ne '51,9,205,53,11'){throw 'Essential rune/door/credit/path must precede optional conversation'}
foreach($pair in @(@(104949,1,1),@(10494900,2,2),@(10494901,2,3))){$r=$sai["$($pair[0])/9/$($pair[1])"];if(!$r -or [int]$r[8] -ne 45 -or [int]$r[9] -ne 7 -or [int]$r[10] -ne $pair[2] -or [int]$r[11] -ne 1){throw 'Native ritual must signal the matching wave'}}
$nativeFile="$out/native.tsv"
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TitanstrikeIntroCatalog.cpp -ProbeName TitanstrikeIntroCatalog -OutputFile $nativeFile
$native=Get-Content $nativeFile
foreach($expected in @("EFFECT`t6660`t0`t2`t0`t0`t6","STEP`t1068`t2`t2301`t47217`t0","CRITERIA`t1068`t47218`t47217`t1`t29709`t92`t49992","EFFECT`t210772`t0`t6`t106342`t0`t1","EFFECT`t210777`t0`t6`t106345`t0`t1")){if($native -notcontains $expected){throw "Native 7.3.5 rune data mismatch $expected"}}
if($VerifyReleaseBinary){
 $binary=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes("$repo/build-extractors/bin/Release/worldserver.exe"))
 foreach($text in @('npc_titanstrike_prustaga','npc_titanstrike_grif','Titanstrike rune wave %u: failed to summon')){if(!$binary.Contains($text)){throw "Canonical Release lacks $text"}}
 $previousPythonPath=$env:PYTHONPATH
 try {
  if(Test-Path "$repo/.codex/pe-review-deps/capstone"){$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPythonPath"}
  & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_titanstrike_runes_release.py" "$repo/build-extractors/bin/Release"
  if($LASTEXITCODE){throw 'Canonical Titanstrike callback instruction check failed'}
 }finally{$env:PYTHONPATH=$previousPythonPath}
}
'PASS: production actor callbacks and native DB2, repeat SQL, ritual delays, script bindings and valid LINK graph. Client appearance, all combat stages and rune/door choreography remain pending.'
