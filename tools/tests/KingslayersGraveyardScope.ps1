# Exact callback is separately compiled with ASAN/native/selected Release proof.
function Restore-KingslayersGraveyardBoundary([string]$source){
 $method=Block $source 'WorldLocation* GetClosestGraveYard('
 $sha=[Security.Cryptography.SHA256]::Create()
 try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($method.Replace("`r",''))))).Replace('-','').ToLowerInvariant()}finally{$sha.Dispose()}
 if($hash -ne '6c431b2fc3a446b437179a50d223c5923b1d078c22d2c71a45a49e2385bd1132'){throw 'Unreviewed Kingslayers cemetery callback'}
 $old=(& git -C $repo show '148c610e4:src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/instance_kingslayers.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned Kingslayers cemetery predecessor missing'}
 return $source.Replace($method,(Block $old 'WorldLocation* GetClosestGraveYard(')).Replace('#include "MapManager.h"','').Replace('#include "Scenario.h"','').Replace('#include "ScenarioMgr.h"','')
}
