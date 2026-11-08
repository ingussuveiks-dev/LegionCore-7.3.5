# Exact independently verified Sharasdal addition; restore the prior cemetery method
# before older unrelated whole-instance comparisons. Unknown edits still fail.
function Remove-SharasdalGraveyardScope([string]$source) {
 if(!$source.Contains('Sharasdal graveyard: invalid native WorldSafeLocs')){return $source}
 function SharasdalCemeteryBlock([string]$text){$a=$text.IndexOf('        WorldLocation* GetClosestGraveYard(');if($a -lt 0){throw 'Missing Sharasdal cemetery scope'};$b=$text.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($text[$b] -eq '{'))-([int]($text[$b] -eq '}'));++$b};return $text.Substring($a,$b-$a)}
 $method=SharasdalCemeteryBlock $source;$bytes=[Text.Encoding]::UTF8.GetBytes(($method -replace '\s',''))
 $hash=([BitConverter]::ToString([Security.Cryptography.SHA256]::Create().ComputeHash($bytes))).Replace('-','').ToLowerInvariant()
 if($hash -ne '14d1aecbe7961895bd8a23d80a32adaacfcaf04439fc40f7fe81e39338db3372'){throw 'Sharasdal cemetery differs from its exact independently verified scope'}
 $prior=(& git -C "$PSScriptRoot/../.." show 'd8cc8bb9e:src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/instance_sharasdal.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned pre-Sharasdal instance absent'}
 return $source.Replace($method,(SharasdalCemeteryBlock $prior)).Replace('#include "MapManager.h"','')
}
