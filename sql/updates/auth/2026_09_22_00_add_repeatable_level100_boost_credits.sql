-- Persist character boosts as a count instead of the legacy one-bit account
-- entitlement. Token type 100 is internal and is deliberately absent from
-- the shop currency table, so it is not displayed as spendable currency.

INSERT INTO `account_tokens` (`account_id`, `tokenType`, `amount`)
SELECT `id`, 100, 1
FROM `account`
WHERE (`AtAuthFlag` & 0x04) <> 0
ON DUPLICATE KEY UPDATE `amount` = GREATEST(`amount`, 1);
