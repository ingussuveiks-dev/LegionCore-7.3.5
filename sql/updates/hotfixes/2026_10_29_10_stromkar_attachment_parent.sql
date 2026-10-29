-- CODE-COUPLED: keep outside the live updater until the matching Release is ready.
-- Complete the native26972 relationship field required by the enabled client store.
-- No visual rows are inserted; the guarded runtime correction changes only318611.
ALTER TABLE spell_visual_kit_model_attach
    ADD COLUMN IF NOT EXISTS SpellVisualKitID int(11) NOT NULL DEFAULT 0 AFTER StartDelay;
