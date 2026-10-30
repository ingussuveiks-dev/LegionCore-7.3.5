param()
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/remaining-artifact-pickup-catalog"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @("$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try {
  $env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf"
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ArtifactRemainingPickupCatalog.cpp -ProbeName ArtifactRemainingPickupCatalog -OutputFile "$out/native.tsv"
 } finally {$env:LEGION_DIALOGUE_CONFIG=$previous}
 $native=Get-Content "$out/native.tsv" -Raw -Encoding UTF8
 foreach($path in @(@(200152,1168,1532,200134,128402),@(190435,1071,1466,186209,128292),@(196023,1125,1500,196020,128820))){
  $spell=$path[0];$scene=$path[1];$package=$path[2];$equip=$path[3];$item=$path[4]
  foreach($line in @("EFFECT`t$spell`t0`t0`t6`t$scene`t0`t1`t0","AURA`t$spell`t0`t430`t0","DURATION`t$spell`t9000","EFFECT`t$equip`t0`t0`t249`t$item`t0`t1`t0","ITEM`t$item`t1`t1`t-1","SCENE_PACKAGE`t$package")){
   if($native -notmatch ('(?m)^'+[regex]::Escape($line)+'\r?$')){throw "Matching native acquisition row differs: $line"}
  }
  $a=$native.IndexOf("SCENE_PACKAGE`t$package");$b=$native.IndexOf('SCENE_PACKAGE', $a+1)
  $text=if($b -lt 0){$native.Substring($a)}else{$native.Substring($a,$b-$a)}
  if($text -notmatch "clone:EquipWeapon\(WeaponSlot.MainHand, $item\)"){throw "Actual native visual clone equip differs: $package"}
  # A local cinematic trigger is not a server inventory grant. Preserve the
  # whole linked package in the probe output for review, including framework.
  $active=($text -split "`n"|Where-Object {$_ -notmatch '^\s*--'}) -join "`n"
  if($active -match 'scene:TriggerServerEvent\s*\('){throw "Native package now has a server callback; review $package"}
 }
 foreach($line in @("CRITERION`t940`t8`t92`t48788`t1","CRITERION`t940`t9`t92`t48184`t1","CRITERION`t940`t10`t92`t48857`t1","CRITERION`t901`t6`t92`t49193`t0","CHILD`t128292`t128293`t16","CHILD`t128820`t133959`t16","EFFECT`t197092`t0`t0`t134`t94390`t0`t1`t0","EFFECT`t197092`t0`t1`t61`t49193`t0`t0`t0","EFFECT`t197092`t0`t2`t167`t0`t0`t1`t0","EFFECT`t197092`t0`t3`t64`t0`t197001`t1`t0","EFFECT`t197092`t0`t4`t249`t128292`t0`t1`t0","EFFECT`t197001`t0`t0`t219`t750`t0`t18`t0")){
  if($native -notmatch ('(?m)^'+[regex]::Escape($line)+'\r?$')){throw "Matching scenario/pair row differs: $line"}
 }
 $create=@($native -split "`n"|Where-Object {$_ -match '^EFFECT\t\d+\t\d+\t\d+\t(?:24|157)\t'})
 if($create.Count -ne 1 -or $create[0].TrimEnd("`r") -cne "EFFECT`t186202`t0`t0`t157`t60768`t0`t1`t0" -or $native -notmatch '(?m)^CREATE_ITEM\t186202\t0\t0\r?$'){throw 'Legacy creation context or scene-root grants differ; review actual ItemType/loot producer'}
 if($native -notmatch '(?m)^EFFECT\t199868\t0\t0\t134\t100813\t0\t1\t0\r?$'){throw 'Frost chest native secondary credit differs'}
 if($native -notmatch '(?m)^FROST_CONVERSATION_LINE\t750\t1784\t97551\t0\t0\t5000\r?$'){throw 'Frost native final conversation line differs'}
 foreach($line in @("CRITERION`t957`t3`t92`t47380`t0","CRITERION`t957`t4`t92`t47042`t0","STEP_REWARD`t957`t4`t0","EFFECT`t196023`t0`t2`t90`t100290`t0`t1`t0","EFFECT`t196023`t0`t3`t64`t0`t196264`t1`t0","EFFECT`t196264`t0`t0`t61`t47042`t0`t0`t0")){
  if($native -notmatch ('(?m)^'+[regex]::Escape($line)+'\r?$')){throw "Matching native Fire path differs: $line"}
 }
 'PASS matching26972 native scenes/existing-item249/unique pairs/full linked clone-only packages; Blood940 and Frost901 events. Extra legacy186202 has ItemType0 and an earlier45608 event, outside190435 trigger closure;199868 only credits100813. Actual SQL loot/scene producers and Fire scenario selection remain separate checks.'
}
