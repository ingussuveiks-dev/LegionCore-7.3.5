param([Parameter(Mandatory)][string]$Python)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path -LiteralPath "$PSScriptRoot/../..").Path
$native = @'
from pathlib import Path
import sys

root = Path(sys.argv[1])
sys.dont_write_bytecode = True
sys.path.insert(0, str(root / 'tools/research'))
from audit_spell_mechanics import Data

dbc = root / 'build-extractors/bin/Release/dbc/enUS'
data = Data(dbc, dbc)
effect, = (row for row in data.rows('SpellEffect').values()
           if row['SpellID'] == 229928 and not row['DifficultyID'])
assert tuple(effect[key] for key in (
    'EffectIndex', 'Effect', 'EffectAura', 'ImplicitTarget1', 'ImplicitTarget2',
    'EffectBasePoints', 'EffectAuraPeriod', 'EffectTriggerSpell',
)) == (0, 6, 8, 38, 0, 92451, 1000, 0)
misc, = (row for row in data.rows('SpellMisc').values()
         if row['SpellID'] == 229928 and not row['DifficultyID'])
assert misc['Attributes2'] & 4
assert data.rows('SpellDuration')[misc['DurationIndex']]['Duration'] == 300000
spell_range = data.rows('SpellRange')[misc['RangeIndex']]
assert spell_range['RangeMax1'] == spell_range['RangeMax2'] == 20
print('PASS build 26972 Funnel Energy target, native periodic healing, channel duration and range')
'@
$native | & $Python - $repo
if ($LASTEXITCODE) { throw 'Agatha Funnel Energy 26972 DB2 regression failed' }
