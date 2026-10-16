-- Mephistroth's opening is emitted as he starts running, 42-49 yards from
-- the doomweavers. SAY's configured 40-yard range lost this one-shot packet
-- (including its voice), leaving silence until the skull pickup dialogue.
-- Use the existing yell delivery range for this opening line only.
UPDATE creature_text SET Type=14
WHERE CreatureID=106692 AND GroupID=0 AND ID=0 AND BroadcastTextID=110315;
