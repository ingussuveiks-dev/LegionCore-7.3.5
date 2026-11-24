# King Phaoris's unrelated quest offer

The player had accepted Thunder on the Sands (40634), but King Phaoris still offered the older Return to the Lost City (28870). The live database identifies Phaoris (47684) as its only starter. Uldum's 28870 turn-in is a separate actor, 50038.

The guarded World migration filters the 28870 offer from the start of Off To Adventure! (40633) through rewarding Into The Heavens (40570). The two condition groups admit players outside that window. The ordinary quest menu and start marker both use source 19 for offered quests, while the already accepted quest can still be turned in through its existing ender. Readback confirmed all four new conditions, the original starter and ender, and the two Monk quest offers. Existing custom acceptance conditions cause the migration to leave the row alone.

The migration was applied to the canonical World database. The running server continues to use its loaded conditions until the next startup. No executable, character data or quest progression was changed. The active client-led workflow uses the next menu visit after startup to confirm the display.
