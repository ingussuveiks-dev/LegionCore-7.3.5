param([switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/gossip-poi"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){if($s[$b] -eq '{'){++$depth};if($s[$b] -eq '}'){--$depth};++$b};$s.Substring($a,$b-$a)}
New-Item -ItemType Directory -Force $out|Out-Null
$evidence=if($Baseline){"$out/baseline"}else{"$out/repaired"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $evidence -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $s=Get-Content "$repo/src/server/game/Globals/GossipData.cpp" -Raw
 if($Baseline){$s=(& git -C $repo show 'ac6b01671:src/server/game/Globals/GossipData.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned POI baseline missing'}}
 [IO.File]::WriteAllText("$out/Loader.inc",(Block $s 'void GossipDataStoreMgr::LoadGossipMenuItems('))
 $s=Get-Content "$repo/src/server/game/Entities/Creature/GossipDef.cpp" -Raw
 [IO.File]::WriteAllText("$out/SendPoi.inc",(Block $s 'void PlayerMenu::SendPointOfInterest('))
 $s=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
 $a=$s.IndexOf('            if (menuItemData->GossipActionPoi)');$b=$s.IndexOf('            break;',$a)
 if($a -lt 0 -or $b -le $a){throw 'Native POI dispatch tail missing'}
 [IO.File]::WriteAllText("$out/Selection.inc",$s.Substring($a,$b-$a))
 $file="$PSScriptRoot/GossipPoiTest.cpp"
 # The shared native builder links original game/shared/common libraries.
 # A dedicated generated source selects the pinned counterfactual only.
 $generated="$repo/tools/research/GossipPoiProbe.generated.cpp"
 try {
  $prefix=if($Baseline){"#define POI_BASELINE`n"}else{''}
  [IO.File]::WriteAllText($generated,$prefix+'#include "../tests/GossipPoiTest.cpp"')
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile GossipPoiProbe.generated.cpp -ProbeName GossipPoiProbe
 }finally{if(Test-Path -LiteralPath $generated){Remove-Item -LiteralPath $generated}}
}
