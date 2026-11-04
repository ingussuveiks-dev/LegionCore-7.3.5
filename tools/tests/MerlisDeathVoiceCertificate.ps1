# The dedicated death suite certifies this exact nested resolver and native
# JustDied factory. Earlier independent Merlis suites retain their boundary.
function Restore-MerlisDeathVoiceBoundary([string]$source) {
 . "$PSScriptRoot/AltheaActorIdentityCertificate.ps1"
 $source=Restore-AltheaActorIdentityBoundary $source
 $actor=Block $source 'class DeathConversation final : public Conversation'
 $source=$source.Replace($actor+';','').Replace('new DeathConversation(me->GetGUID())','new Conversation')
 $source=[regex]::Replace($source,'(?m)^        // Native actor lookup searches for a living creature\. JustDied must\r?\n        // retain this actual dead Merlis, not a nearby living duplicate\.\r?\n','')
 return $source
}
