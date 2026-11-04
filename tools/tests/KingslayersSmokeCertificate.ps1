# Earlier independent source certificates retain their pre-smoke boundary.
function Restore-KingslayersSmokeBoundary([string]$source) {
 $old=(& git -C $repo show '81c43cc8c:src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned smoke boundary missing'}
 return $source.Replace((Block $source 'class spell_kingslayers_smoke_bomb : public SpellScriptLoader'),(Block $old 'class spell_kingslayers_smoke_bomb : public SpellScriptLoader'))
}
