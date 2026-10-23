$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/talonclaw-return"
New-Item -ItemType Directory -Force $out | Out-Null
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SurvivalReturnCatalog.cpp -ProbeName SurvivalReturnCatalog -OutputFile "$out/native.tsv"
$rows=Get-Content "$out/native.tsv"
foreach($expected in @("AREA`t7502`t1220`tDalaran","EFFECT`t197651`t1`t90`t0`t100112`t0`t1`t0`t0","EFFECT`t205575`t0`t134`t0`t103798`t0`t1`t0`t0","EFFECT`t205575`t1`t6`t4`t0`t0`t1`t0`t0")){
 if($rows -notcontains $expected){throw "Survival native return mismatch: $expected"}
}
$source=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
if($source -notmatch 'EffectKillCreditPersonal,\s*// 90' -or $source -notmatch 'EffectKillCredit,\s*//134'){
 throw 'Native personal/group kill-credit effect dispatch changed'
}
'PASS: actual26972 Dalaran area/map, personal197651 claim and distinct group-wide205575 return effect; do not cast the latter to leak arrival credit'
$teleport=@($rows | Where-Object {$_ -like "EFFECT`t190598`t*"})
if($teleport.Count -ne 1 -or $teleport[0] -ne "EFFECT`t190598`t0`t252`t0`t1317`t0`t1`t17`t0"){throw 'Generic native Dalaran teleport acquired extra/changed effects'}
if(!($rows | Where-Object {$_ -like "PORTAL_MODEL`t8111`t*"})){throw 'Native Dalaran portal model missing'}
foreach($sound in @(62702,62703)){
 if(!($rows | Where-Object {$_ -like "VOICE`t$sound`t*"})){throw "Native Dorro voice file missing: $sound"}
}
'PASS: native8111 portal model and pure190598 teleport, no extra quest/group credit effect'
foreach($kind in @('World','Hotfix')){
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match "^${kind}DatabaseInfo\s*="}).Split('"')[1].Split(';')
 $sql=if($kind -eq 'World'){
  "SELECT COUNT(*) FROM spell_target_position WHERE id=190598 AND target_map=1220 AND target_position_x BETWEEN -830 AND -827 AND target_position_y BETWEEN 4370 AND 4373 AND target_position_z BETWEEN 737 AND 740;"
 }else{
  'SELECT COUNT(*) FROM broadcast_text WHERE (ID=103406 AND SoundEntriesID1=62702) OR (ID=106397 AND SoundEntriesID1=62703);'
 }
 $previous=$env:MYSQL_PWD
 try {
  $env:MYSQL_PWD=$connection[3]
  $result=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
  if($LASTEXITCODE){throw 'Native departure data read failed'}
 }finally{$env:MYSQL_PWD=$previous}
 $expected=if($kind -eq 'World'){'1'}else{'2'}
 if($result.Count -ne 1 -or $result[0] -ne $expected){throw "Missing native $kind departure data"}
}
'PASS: actual native Krasus landing destination and both local hotfix Dorro broadcasts/sounds; this does not establish the retail portal entry or observed sound playback'
