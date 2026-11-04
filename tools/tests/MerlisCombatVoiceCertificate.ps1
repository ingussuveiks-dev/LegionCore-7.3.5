# Earlier independent intro/handoff/owner certificates retain their original
# combat boundary. Test-MerlisCombatVoice separately certifies the whole-file
# combat delta and executes both original and repaired combat tails.
function Restore-MerlisCombatVoiceBoundary([string]$source) {
 . "$PSScriptRoot/MerlisDeathVoiceCertificate.ps1"
 $source=Restore-MerlisDeathVoiceBoundary $source
 $old=(& git -C $repo show 'd8c85e9fe:src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned independently tested combat boundary missing'}
 $ai=Block $source 'struct npc_merlis_malaganAI : ScriptedAI';$prior=Block $old 'struct npc_merlis_malaganAI : ScriptedAI'
 $start=$ai.IndexOf('        uint32 combatVoiceDelay');$last=Block $ai 'void RequestCombatVoice(';$end=$ai.IndexOf($last,$start)+$last.Length
 if($start -lt 0 -or $end -le $start){throw 'Dedicated combat voice boundary absent'}
 $normalized=$ai.Remove($start,$end-$start)
 foreach($method in @('void Reset(','void EnterCombat(','void UpdateAI(')){$normalized=$normalized.Replace((Block $ai $method),(Block $prior $method))}
 return $source.Replace($ai,$normalized)
}
