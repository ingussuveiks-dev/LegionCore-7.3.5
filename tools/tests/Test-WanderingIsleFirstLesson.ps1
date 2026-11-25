param(
    [string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/wandering-isle-first-lesson",
    [switch]$VerifyLiveData
)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration = Get-Content -LiteralPath "$repo/sql/updates/world/2026_11_25_14_wandering_isle_first_lesson.sql" -Raw
$completion = Get-Content -LiteralPath "$repo/sql/updates/world/2026_11_25_15_wandering_isle_weapon_lesson_completion.sql" -Raw
$check = @'
SELECT 'quiet-area',COUNT(*) FROM area_queststart WHERE id=5834 AND quest BETWEEN 30039 AND 30045;
SELECT 'old-prerequisites',COUNT(*) FROM quest_template_addon
 WHERE (ID=30027 OR ID BETWEEN 30033 AND 30038) AND PrevQuestID<>0;
SELECT 'incoming-prerequisites',COUNT(*) FROM quest_template_addon WHERE ID BETWEEN 30039 AND 30045 AND NextQuestID<>0;
SELECT 'lesson-offers',COUNT(*) FROM creature_queststarter WHERE id=53566 AND (quest=30027 OR quest BETWEEN 30033 AND 30038);
SELECT 'class-and-next-lesson',COUNT(*) FROM quest_template_addon AS a JOIN quest_template AS q ON q.ID=a.ID
 WHERE a.ID IN (30027,30033,30034,30035,30036,30037,30038) AND a.NextQuestID=29406
 AND a.AllowableClasses=CASE a.ID WHEN 30027 THEN 512 WHEN 30033 THEN 128 WHEN 30034 THEN 4
     WHEN 30035 THEN 16 WHEN 30036 THEN 8 WHEN 30037 THEN 64 WHEN 30038 THEN 1 END
 AND q.RewardNextQuest=29406 AND (q.AllowableRaces & 8388608)<>0;
SELECT 'kept-history',COUNT(*) FROM quest_template AS q JOIN creature_questender AS giver ON giver.quest=q.ID
 WHERE q.ID BETWEEN 30039 AND 30045 AND giver.id=53566
 AND q.RewardNextQuest=CASE WHEN q.ID=30039 THEN 30027 ELSE q.ID-7 END;
SELECT 'visibility-rules',COUNT(*) FROM spell_area WHERE spell=56267 AND area=5834 AND quest_end=29409
 AND quest_start_status=74 AND quest_end_status=8 AND autocast=1
 AND (quest_start=30027 OR quest_start BETWEEN 30033 AND 30045);
SELECT 'lesson-completion-flags',COUNT(*) FROM quest_template_addon
 WHERE (ID=30027 OR ID BETWEEN 30033 AND 30038) AND (SpecialFlags & 17)=16;
'@

& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $OutputDirectory -Test {
    $query = if ($VerifyLiveData) {
        $check
    } else {
@"
CREATE TEMPORARY TABLE backup_area LIKE area_queststart;
INSERT INTO backup_area SELECT * FROM area_queststart WHERE id=5834;
CREATE TEMPORARY TABLE area_queststart LIKE backup_area;
INSERT INTO area_queststart SELECT * FROM backup_area;
INSERT INTO area_queststart VALUES (5835,30039);
CREATE TEMPORARY TABLE backup_addon LIKE quest_template_addon;
INSERT INTO backup_addon SELECT * FROM quest_template_addon WHERE ID=30027 OR ID BETWEEN 30033 AND 30045;
CREATE TEMPORARY TABLE quest_template_addon LIKE backup_addon;
INSERT INTO quest_template_addon SELECT * FROM backup_addon;
CREATE TEMPORARY TABLE backup_spell LIKE spell_area;
INSERT INTO backup_spell SELECT * FROM spell_area WHERE spell=56267;
CREATE TEMPORARY TABLE spell_area LIKE backup_spell;
INSERT INTO spell_area SELECT * FROM backup_spell;
UPDATE quest_template_addon SET NextQuestID=CASE WHEN ID=30039 THEN 30027 ELSE ID-7 END WHERE ID BETWEEN 30039 AND 30045;
UPDATE quest_template_addon SET PrevQuestID=CASE WHEN ID=30027 THEN 30039 ELSE ID+7 END WHERE ID=30027 OR ID BETWEEN 30033 AND 30038;
INSERT INTO area_queststart VALUES (5834,30039),(5834,30040),(5834,30041),(5834,30042),(5834,30043),(5834,30044),(5834,30045)
 ON DUPLICATE KEY UPDATE quest=VALUES(quest);
DELETE FROM spell_area WHERE spell=56267 AND area=5834 AND (quest_start=30027 OR quest_start BETWEEN 30033 AND 30038);
UPDATE quest_template_addon SET SpecialFlags=17 WHERE ID=30027 OR ID BETWEEN 30033 AND 30038;
UPDATE quest_template_addon SET SpecialFlags=49 WHERE ID IN (30027,30039);
SELECT 'baseline',COUNT(*) FROM area_queststart WHERE id=5834 AND quest BETWEEN 30039 AND 30045;
$migration
$completion
SELECT 'completion-changes',ROW_COUNT();
$check
$migration
$completion
SELECT 'completion-changes',ROW_COUNT();
$check
SELECT 'preserved-lesson-flag',SpecialFlags FROM quest_template_addon WHERE ID=30027;
SELECT 'preserved-legacy-flags',SpecialFlags FROM quest_template_addon WHERE ID=30039;
SELECT 'other-area',COUNT(*) FROM area_queststart WHERE id=5835 AND quest=30039;
UPDATE quest_template_addon SET NextQuestID=30027 WHERE ID=30039;
UPDATE quest_template_addon SET PrevQuestID=12345 WHERE ID=30027;
UPDATE spell_area SET quest_end_status=1 WHERE spell=56267 AND area=5834 AND quest_start=30027;
INSERT INTO area_queststart VALUES (5834,30039);
$migration
SELECT 'custom-path',COUNT(*) FROM area_queststart WHERE id=5834 AND quest=30039;
SELECT 'custom-prerequisite',PrevQuestID FROM quest_template_addon WHERE ID=30027;
SELECT 'custom-visibility',quest_end_status FROM spell_area WHERE spell=56267 AND area=5834 AND quest_start=30027;
"@
    }
    $connection = (Get-Content -LiteralPath "$repo/build-extractors/bin/Release/worldserver.conf" |
        Where-Object { $_ -match '^WorldDatabaseInfo\s*=' }).Split('"')[1].Split(';')
    $arguments = @("--host=$($connection[0])", "--port=$($connection[1])", "--user=$($connection[2])",
        "--database=$($connection[4])", '--batch', '--raw', '--skip-column-names')
    $previous = $env:MYSQL_PWD
    try {
        $env:MYSQL_PWD = $connection[3]
        $rows = @($query | & 'C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mariadb.exe' @arguments)
        if ($LASTEXITCODE) { throw 'Wandering Isle data query failed' }
    } finally { $env:MYSQL_PWD = $previous }
    $expected = 'quiet-area 0','old-prerequisites 0','incoming-prerequisites 0',
        'lesson-offers 7','class-and-next-lesson 7','kept-history 7','visibility-rules 14','lesson-completion-flags 7'
    if (!$VerifyLiveData) {
        $expected = @('baseline 7','completion-changes 7') + $expected + @('completion-changes 0') + $expected +
            @('preserved-lesson-flag 48','preserved-legacy-flags 49','other-area 1','custom-path 1','custom-prerequisite 12345','custom-visibility 1')
    }
    if ((($rows -replace '\t',' ') -join '|') -cne ($expected -join '|')) {
        $rows
        throw 'Wandering Isle prerequisites, history or aura gates differ from expected'
    }
    $rows
}
