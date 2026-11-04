# Earlier independent suites retain their actor boundary; the dedicated
# identity ASAN/native/whole-file suite certifies the complete new subtype.
function Restore-AltheaActorIdentityBoundary([string]$source) {
 $old=(& git -C $repo show '717261c1a:src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned Althea actor identity boundary missing'}
 return $source.Replace((Block $source 'class AltheaDeathConversation final : public Conversation'),(Block $old 'class AltheaDeathConversation final : public Conversation'))
}
