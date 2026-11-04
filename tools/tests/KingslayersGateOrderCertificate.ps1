# Independent earlier suites retain the original shared-small-events body.
# Its scoped gate-order delta has its own whole-source/ASAN/native certificate.
function Restore-KingslayersGateOrderBoundary([string]$source) {
 $old=(& git -C $repo show 'ff238f715:src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned Clay gate-order boundary missing'}
 return $source.Replace((Block $source 'class npc_kingslayers_small_events : public CreatureScript'),(Block $old 'class npc_kingslayers_small_events : public CreatureScript'))
}
