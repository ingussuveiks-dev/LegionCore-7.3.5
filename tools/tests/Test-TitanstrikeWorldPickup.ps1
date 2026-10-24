param([switch]$VerifyReleaseBinary,[switch]$Baseline)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/titanstrike-world-pickup"
New-Item -ItemType Directory -Force $out | Out-Null
& "$PSScriptRoot/Test-TalonclawWorldPickup.ps1" -Titanstrike -BaselineTitanstrike:$Baseline -VerifyReleaseBinary:$VerifyReleaseBinary
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TitanstrikeTheftCatalog.cpp -ProbeName TitanstrikeTheftCatalog -OutputFile "$out/native.tsv"
$native=Get-Content "$out/native.tsv"
foreach($expected in @("EFFECT`t212010`t0`t64`t0`t0`t0`t212012`t0`t0","EFFECT`t212010`t1`t249`t0`t128861`t0`t0`t1`t0","EFFECT`t212010`t2`t6`t430`t1270`t0`t0`t1`t0","EFFECT`t212012`t0`t90`t0`t114509`t0`t0`t1`t0")){
 if($native -notcontains $expected){throw "Native Titanstrike claim metadata mismatch: $expected"}
}
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SurvivalArtifactCatalog.cpp -ProbeName SurvivalArtifactCatalog -OutputFile "$out/scenes.tsv"
$scenes=Get-Content "$out/scenes.tsv" -Raw
foreach($expected in @("SCENE_TEXT`t14113`t","SCENE_TEXT`t14114`t","SCENE_TEXT`t14813`t",'WEAPONID = 128861','WEAPONTYPE = 4','WEAPONEFFECT = 7','scene:SetFadeRegion(200, true)')){
 if(!$scenes.Contains($expected)){throw "Native Titanstrike scene metadata mismatch: $expected"}
}
$sql=@'
SELECT 'binding',AIName,ScriptName FROM gameobject_template WHERE entry=249812;
SELECT 'scene',SceneScriptPackageID,PlaybackFlags FROM spell_scene WHERE MiscValue=1270;
SELECT 'pair',COUNT(*),ROUND(MAX(CASE WHEN action_type=12 THEN target_z END)-MAX(CASE WHEN action_type=50 THEN target_z END),2),ROUND(SQRT(POW(MAX(CASE WHEN action_type=12 THEN target_x END)-MAX(CASE WHEN action_type=50 THEN target_x END),2)+POW(MAX(CASE WHEN action_type=12 THEN target_y END)-MAX(CASE WHEN action_type=50 THEN target_y END),2)+POW(MAX(CASE WHEN action_type=12 THEN target_z END)-MAX(CASE WHEN action_type=50 THEN target_z END),2)),2) FROM smart_scripts WHERE entryorguid=103154 AND source_type=9 AND ((id=5 AND action_type=12 AND action_param1=106745 AND action_param2=8) OR (id=6 AND action_type=50 AND action_param1=249812)) AND target_type=8;
SELECT 'claim',COUNT(*) FROM smart_scripts WHERE entryorguid=249812 AND source_type=1 AND ((id=0 AND link=1 AND event_type=70 AND event_param1=3 AND action_type=85 AND action_param1=212010 AND target_type=7) OR (id=1 AND link=3 AND event_type=61 AND action_type=205 AND action_param1=92 AND action_param2=50923 AND target_type=7) OR (id=3 AND link=4 AND event_type=61 AND action_type=12 AND action_param1=106854) OR (id=4 AND link=0 AND event_type=61 AND action_type=28 AND action_param1=211970 AND target_type=7));
SELECT 'other-despawn',COUNT(*) FROM smart_scripts WHERE entryorguid=106745 AND source_type=0 AND event_type=38 AND event_param1=1 AND event_param2=1 AND action_type=41;
'@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$connection[3]
 $rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Read-only Titanstrike world pickup audit failed'}
}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("binding`tSmartGameObjectAI`tgo_artifact_acquisition","scene`t1622`t21","pair`t2`t4.72`t4.81","claim`t4","other-despawn`t1")){
 if($rows -notcontains $expected){throw "Live Titanstrike visual lifecycle mismatch: $expected"}
}
$rows | Set-Content -Encoding utf8 "$out/live.tsv"
'PASS: native212012 is personal quest credit, not prop removal; Hati103154 summons both chest and elevated visual4.81yd apart. Original claim/scene/equip/criteria/bird and earlier prop dispatcher remain unchanged. Client render/timing/equip remain pending.'
