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
