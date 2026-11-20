param([Parameter(Mandatory=$true)][string]$FixtureOptionsFile)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath "$PSScriptRoot/../..").Path
$migration = Get-Content -LiteralPath "$root/sql/updates/world/2026_11_20_02_feltotem_owner_credit.sql" -Raw
$maria = 'C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mariadb.exe'
$options = (Resolve-Path -LiteralPath $FixtureOptionsFile).Path
$selector = 'source_type=0 AND ((entryorguid=117230 AND id=7) OR (entryorguid=117484 AND id IN (14,15)))'
$readCredit = "SELECT CONCAT(entryorguid,':',id,':',link,':',action_type,':',action_param1,':',action_param2,':',target_type,':',target_param1) FROM smart_scripts WHERE $selector ORDER BY entryorguid,id;"
$readClasses = 'SELECT GROUP_CONCAT(ConditionValue1 ORDER BY ElseGroup SEPARATOR ":") FROM conditions WHERE SourceTypeOrReferenceId=19 AND SourceEntry=45627 AND ConditionTypeOrReference=15;'
$readGuards = @'
SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=19 AND SourceEntry=45627 AND ConditionTypeOrReference=8 AND ConditionValue1=45842 AND NegativeCondition=0;
SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND ConditionTypeOrReference=29 AND ConditionValue2=100 AND NegativeCondition=1 AND ((SourceEntry=117230 AND SourceGroup=8 AND ConditionValue1=117484) OR (SourceEntry=117484 AND SourceGroup=15 AND ConditionValue1=117230));
SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=117871 AND SourceGroup IN(85,86) AND ConditionTypeOrReference=55 AND ConditionValue1=256;
'@
$data = @"
CREATE TEMPORARY TABLE original_smart AS SELECT * FROM smart_scripts WHERE source_type=0 AND entryorguid IN(117230,117484,117871);
CREATE TEMPORARY TABLE original_conditions AS SELECT * FROM conditions WHERE SourceEntry IN(45627,117230,117484,117871);
CREATE TEMPORARY TABLE smart_scripts LIKE original_smart;
CREATE TEMPORARY TABLE conditions LIKE original_conditions;
INSERT INTO smart_scripts SELECT * FROM original_smart;
INSERT INTO conditions SELECT * FROM original_conditions;
$readCredit
$readClasses
$migration
$readCredit
$readClasses
$readGuards
$migration
$readCredit
$readClasses
"@
$rows = @($data | & $maria "--defaults-extra-file=$options" --database=legion_world --batch --raw --skip-column-names)
if ($LASTEXITCODE) { throw 'Private Feltotem migration regression failed' }
$expected = @(
    '117230:7:0:85:233870:0:16:0','117484:14:16:85:233870:0:16:0','117484:15:0:205:92:58088:16:0','256:512:4:256',
    '117230:7:0:85:233870:128:130:100','117484:14:16:85:233870:128:130:100','117484:15:0:205:92:58088:130:100','256:512:4:16',
    '4','2','2',
    '117230:7:0:85:233870:128:130:100','117484:14:16:85:233870:128:130:100','117484:15:0:205:92:58088:130:100','256:512:4:16'
)
if ($rows.Count -ne $expected.Count -or (Compare-Object $rows $expected -SyncWindow 0)) {
    $rows
    throw 'Feltotem credit, prerequisites, or idempotence did not match'
}
'COUNTERFACTUAL: original Feltotem death rows exclude pet owners and omit the Priest class.'
'PASS: the three repaired actions select the actual player, preserve both boss-death guards and admit Discipline after the original prerequisite.'
