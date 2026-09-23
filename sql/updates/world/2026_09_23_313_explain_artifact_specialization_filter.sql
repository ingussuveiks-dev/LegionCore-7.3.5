-- Explain why an artifact may disappear from the in-world shop after a
-- specialization change.  Product Name2 is consistently formatted as
-- "Class - Specialization artifact", so keep the displayed specialization
-- name and the eligibility rule in sync without duplicating 36 name lists.

START TRANSACTION;

UPDATE `battlepay_display_info`
SET `Name3` = CONCAT(
    'Requires the ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`Name2`, ' artifact', 1), ' - ', -1),
    ' specialization to be active. After changing specialization, reopen the shop. ',
    `Name3`)
WHERE `DisplayInfoId` BETWEEN 111 AND 146
  AND `Name3` NOT LIKE 'Requires the % specialization to be active.%';

COMMIT;
