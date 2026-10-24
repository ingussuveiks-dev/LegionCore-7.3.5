param()
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warswords-helya-visual-data"
New-Item -ItemType Directory -Force $out|Out-Null
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile WarswordsHelyaVisualCatalog.cpp -ProbeName WarswordsHelyaVisualCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native Helya visual graph failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv"
foreach($expected in @("SOURCE`troot-tables-base-plus-hotfix`tclient-parent-records-base-only","ACTOR`t66238`t8558`t1266228`t0`t11`t1","ATTRIBUTES`t195062`t536871296`t805307396`t1","LINK`t195062`t90089`t50629`t0`t0`t0`t0`t0`t1","LINK`t205424`t95925`t53867`t0`t0`t0`t0`t0`t1","ANIMATION`t60379`t23201`t65535`t65535`t9034","ANIMATION`t60384`t23208`t65535`t124`t0","PROCEDURE`t60384`t8564`t26`t4386`t0`t0`t3","ATTACHMENT`t65227`t319325`t24234`t255`t0`t65535`t65535`t65535`t7`t366760`t0.3`t0")){if($native -notcontains $expected){throw "Helya visual record changed: $expected"}}
if(@($native|Where-Object {$_ -match '^EVENT\t'}).Count -ne 5){throw 'Helya native visual event graph changed'}
foreach($expected in @("EVENT`t50629`t104428`t7`t8`t1`t21999`t0`t0`t0`t0","EVENT`t50629`t104429`t7`t8`t2`t60379`t0`t0`t0`t0","EVENT`t50629`t104430`t8`t13`t2`t60380`t0`t0`t0`t0","EVENT`t50629`t104431`t11`t12`t2`t60384`t0`t0`t0`t0","EVENT`t53867`t114640`t7`t8`t2`t65227`t0`t0`t0`t0","ANIMKIT`t9034`t0`t0`t0","ANIM_SEGMENT`t9034`t17309`t55`t230`t0`t0","ANIM_SEGMENT`t9034`t17310`t0`t0`t0`t0","ANIM_SEGMENT`t9034`t17311`t0`t0`t0`t0")){if($native -notcontains $expected){throw "Helya event/animation relationship changed: $expected"}}
foreach($kit in @(76006,76033,76034,76031)){if(!($native|Where-Object {$_ -match "^AUDIO`t\d+`t$kit`t\d+`t[1-9]\d*`t"})){throw "Helya sound $kit has no native file"}}
$types=@($native|Where-Object {$_ -match '^KIT_EFFECT\t'}|ForEach-Object {($_ -split "`t")[3]}|Sort-Object -Unique)
if(($types -join ',') -ne '1,2,5,6'){throw 'Helya kit effect types require review'}
'PASS: actual native SpellVisual table now expands;195062/205424 links, five visual events, typed kit/attachments/model files, procedure, animation and sound references resolved. Client-only parent/name records are base-only; model bytes, actor targeting, timing/audio and rendered client visuals remain unverified.'
