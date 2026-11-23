param([Parameter(Mandatory=$true)][string]$OutputDirectory)

$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force $out | Out-Null

function Export([string]$source,[string]$start,[string]$end,[string]$name){
 $a=$source.IndexOf($start);$b=$source.IndexOf($end,$a)
 if($a -lt 0 -or $b -le $a){throw "Missing native POI boundary $name"}
 [IO.File]::WriteAllText("$out/$name.inc",$source.Substring($a,$b-$a))
}
$header=Get-Content "$repo/src/server/game/Globals/QuestData.h" -Raw
Export $header 'struct QuestPOIPoint' 'struct PointOfInterest' 'PoiTypes'
$native=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
Export $native 'QuestPOI::QuestPOI()' 'QuestDataStoreMgr::QuestDataStoreMgr()' 'PoiConstructors'
Export $native 'void QuestDataStoreMgr::LoadQuestPOI()' 'void QuestDataStoreMgr::LoadQuestRelationsHelper(' 'PoiLoader'
Export $native 'QuestPOIVector const* QuestDataStoreMgr::GetQuestPOIVector(' 'std::vector<QuestObjective> QuestDataStoreMgr::GetQuestObjectivesByType(' 'PoiLookup'
$handler=Get-Content "$repo/src/server/game/Handlers/QueryHandler.cpp" -Raw
$a=$handler.IndexOf('            WorldPackets::Query::QuestPOIData questPOIData;');$end='            response.QuestPOIDataStats.push_back(questPOIData);';$b=$handler.IndexOf($end,$a)
if($a -lt 0 -or $b -le $a){throw 'Missing native response construction'}
[IO.File]::WriteAllText("$out/PoiResponse.inc",$handler.Substring($a,$b-$a+$end.Length))
