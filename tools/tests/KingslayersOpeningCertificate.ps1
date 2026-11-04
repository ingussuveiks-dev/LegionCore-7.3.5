# The full opening AI/source/native-criteria suite certifies this delta.
# Independent later-actor suites retain their previously verified boundary.
function Restore-KingslayersOpeningBoundary([string]$source) {
 . "$PSScriptRoot/KingslayersGateOrderCertificate.ps1"
 $source=Restore-KingslayersGateOrderBoundary $source
 $old=(& git -C $repo show '0d5a1628e:src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned opening boundary missing'}
 return $source.Replace((Block $source 'struct npc_garona_halforcenAI : ScriptedAI'),(Block $old 'struct npc_garona_halforcenAI : ScriptedAI'))
}
