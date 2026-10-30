-- Keep pending until the quarter-scale attachment implementation is promoted.
-- Generated notice76 was already accepted by the client with Scale1.0.
-- A new persistent ID requests the corrected full record without deleting cache.
START TRANSACTION;
SET @stromkar_blade_notice_count=(SELECT COUNT(*) FROM hotfix_data WHERE TableHash=4033975491 AND RecordId=318611);
SET @stromkar_blade_notice_safe=(@stromkar_blade_notice_count<=1 AND NOT EXISTS(
 SELECT 1 FROM hotfix_data WHERE TableHash=4033975491 AND RecordId=318611 AND (Deleted<>0 OR Id<=76)));
SELECT COALESCE(MAX(Id),0)+1 INTO @stromkar_blade_notice_id FROM hotfix_data FOR UPDATE;
SET @stromkar_blade_notice_safe=(@stromkar_blade_notice_safe AND
 (@stromkar_blade_notice_count=1 OR @stromkar_blade_notice_id BETWEEN 77 AND 2147483647));
SET @stromkar_blade_notice_guard=IF(@stromkar_blade_notice_safe,'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Stromkar blade cache notice differs; review before applying''');
PREPARE stromkar_blade_notice_check FROM @stromkar_blade_notice_guard;
EXECUTE stromkar_blade_notice_check;
DEALLOCATE PREPARE stromkar_blade_notice_check;
INSERT INTO hotfix_data(Id,TableHash,RecordId,Deleted)
SELECT @stromkar_blade_notice_id,4033975491,318611,0
WHERE @stromkar_blade_notice_safe AND @stromkar_blade_notice_count=0;
COMMIT;
