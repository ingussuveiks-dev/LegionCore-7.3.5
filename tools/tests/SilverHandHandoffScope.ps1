# Exact independently verified Holy addition; restore the prior Shadow method
# before older unrelated whole-instance comparisons. Unknown edits still fail.
function Remove-HolyGraveyardScope([string]$source) {
 if(!$source.Contains('Silver Hand graveyard: invalid native WorldSafeLocs')){return $source}
 function HolyCemeteryBlock([string]$text){$a=$text.IndexOf('        WorldLocation* GetClosestGraveYard(');if($a -lt 0){throw 'Missing Holy cemetery scope'};$b=$text.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($text[$b] -eq '{'))-([int]($text[$b] -eq '}'));++$b};return $text.Substring($a,$b-$a)}
 $method=HolyCemeteryBlock $source;$bytes=[Text.Encoding]::UTF8.GetBytes(($method -replace '\s',''))
 $hash=([BitConverter]::ToString([Security.Cryptography.SHA256]::Create().ComputeHash($bytes))).Replace('-','').ToLowerInvariant()
 if($hash -ne '0cc1f37f7efe3e5c24c213c6e7ae2ad892e4f3beea3b12095682ef63c4d810a7'){throw 'Holy cemetery differs from its exact independently verified scope'}
 $prior=(& git -C "$PSScriptRoot/../.." show '2bd533cfd:src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/instance_silver_hand.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned pre-Holy instance absent'}
 return $source.Replace($method,(HolyCemeteryBlock $prior))
}
# Exact independently verified Shadow addition; restore the prior Arms method
# before older unrelated whole-instance comparisons. Unknown edits still fail.
function Remove-ShadowGraveyardScope([string]$source) {
 $source=Remove-HolyGraveyardScope $source
 if(!$source.Contains('Shadow graveyard: invalid native WorldSafeLocs')){return $source}
 function ShadowCemeteryBlock([string]$text){$a=$text.IndexOf('        WorldLocation* GetClosestGraveYard(');if($a -lt 0){throw 'Missing Shadow cemetery scope'};$b=$text.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($text[$b] -eq '{'))-([int]($text[$b] -eq '}'));++$b};return $text.Substring($a,$b-$a)}
 $method=ShadowCemeteryBlock $source;$bytes=[Text.Encoding]::UTF8.GetBytes(($method -replace '\s',''))
 $hash=([BitConverter]::ToString([Security.Cryptography.SHA256]::Create().ComputeHash($bytes))).Replace('-','').ToLowerInvariant()
 if($hash -ne '12ee9385c1543789bee6250d40fd8b65a3a0037a42c61fd6b6f9f566aabc85b0'){throw 'Shadow cemetery differs from its exact independently verified scope'}
 $prior=(& git -C "$PSScriptRoot/../.." show '41e2b7743:src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/instance_silver_hand.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned pre-Shadow instance absent'}
 return $source.Replace($method,(ShadowCemeteryBlock $prior))
}
# Preserve the exact, separately tested Arms cemetery addition while comparing
# earlier whole-instance changes. Unknown callback edits still fail this guard.
function Remove-ArmsGraveyardScope([string]$source) {
 $source=Remove-ShadowGraveyardScope $source
 if(!$source.Contains('Stromkar graveyard: invalid native WorldSafeLocs')){return $source}
 function CemeteryBlock([string]$text){$a=$text.IndexOf('        WorldLocation* GetClosestGraveYard(');if($a -lt 0){throw 'Missing cemetery scope'};$b=$text.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($text[$b] -eq '{'))-([int]($text[$b] -eq '}'));++$b};return $text.Substring($a,$b-$a)}
 $method=CemeteryBlock $source
 $bytes=[Text.Encoding]::UTF8.GetBytes(($method -replace '\s',''))
 $hash=([BitConverter]::ToString([Security.Cryptography.SHA256]::Create().ComputeHash($bytes))).Replace('-','').ToLowerInvariant()
 if($hash -ne 'f0f8d5ed0b93b767a1997c0cebcbb66cb26a8586bbc0e0cec553eefdb335edb3'){throw 'Arms cemetery differs from the exact independently verified scope'}
 $prior=(& git -C "$PSScriptRoot/../.." show '96ca7646c:src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/instance_silver_hand.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned pre-Arms instance absent'}
 return $source.Replace($method,(CemeteryBlock $prior)).Replace('#include "MapManager.h"','')
}
# Verify the exact Priest-only addition before older whole-instance comparisons.
function Remove-SilverHandHandoffScope([string]$source) {
 $source=Remove-ArmsGraveyardScope $source
 $source=$source.Replace("`r",'')
 $include='#include "../../Priest/slaghammer_handoff.h"'
 $base='struct instance_silver_hand_InstanceMapScript : public InstanceScript, public SlaghammerHandoff::Instance'
 $enter=@'
        void OnPlayerEnter(Player* player) override
        {
            slaghammer.Rejoin(player);
        }
'@
 $actor=@'
class npc_slaghammer_handoff : public CreatureScript
{
public:
    npc_slaghammer_handoff() : CreatureScript("npc_slaghammer_handoff") { }
    CreatureAI* GetAI(Creature* creature) const override { return new SlaghammerHandoff::AI(creature); }
};
'@
 foreach($part in @($include,$base,$enter,$actor,'    new npc_slaghammer_handoff();')) {
  if(!$source.Contains($part.Replace("`r",''))){throw 'Exact Priest instance-owned handoff binding differs'}
 }
 $source=$source.Replace($include,'').Replace($base,'struct instance_silver_hand_InstanceMapScript : public InstanceScript').Replace($enter.Replace("`r",''),'').Replace($actor.Replace("`r",''),'').Replace('    new npc_slaghammer_handoff();','')
 $pinned=(& git -C "$PSScriptRoot/../.." show 'dafaace6d:src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/instance_silver_hand.cpp') -join "`n"
 if($LASTEXITCODE -or ($source -replace '\s','') -cne ($pinned -replace '\s','')){throw 'Shared instance changed beyond the exact Priest handoff addition'}
 return $source
}
