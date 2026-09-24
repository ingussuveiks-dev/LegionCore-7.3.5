-- Shop artifacts can be equipped before level 110, but their first trait is
-- unlocked for 100 Artifact Power at a class-hall artifact forge. Make that
-- step clear in the offer.
UPDATE `battlepay_display_info`
SET `Name3` = REPLACE(REPLACE(
    `Name3`,
    ' without completing its acquisition quest chain.',
    '. Earn 100 Artifact Power, then visit your class hall artifact forge to unlock the first trait.'),
    '. Visit your class hall artifact forge to begin upgrading it.',
    '. Earn 100 Artifact Power, then visit your class hall artifact forge to unlock the first trait.')
WHERE `DisplayInfoId` BETWEEN 111 AND 146
  AND (`Name3` LIKE '% without completing its acquisition quest chain.'
       OR `Name3` LIKE '%Visit your class hall artifact forge to begin upgrading it.');
