-- These child-pool links have no pool_template and no creature, gameobject,
-- quest, or nested-pool members.  PoolMgr already ignores all four links.
CREATE TABLE IF NOT EXISTS `_backup_20260918_empty_child_pools`
LIKE `pool_pool`;

REPLACE INTO `_backup_20260918_empty_child_pools`
SELECT *
FROM `pool_pool`
WHERE (`pool_id` = 5132 AND `mother_pool` = 5122)
   OR (`pool_id` = 5145 AND `mother_pool` = 5122)
   OR (`pool_id` = 5618 AND `mother_pool` = 896)
   OR (`pool_id` = 11417 AND `mother_pool` = 9908);

DELETE FROM `pool_pool`
WHERE (`pool_id` = 5132 AND `mother_pool` = 5122)
   OR (`pool_id` = 5145 AND `mother_pool` = 5122)
   OR (`pool_id` = 5618 AND `mother_pool` = 896)
   OR (`pool_id` = 11417 AND `mother_pool` = 9908);
