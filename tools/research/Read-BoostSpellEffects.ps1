param(
    [string]$Db2Path = "$PSScriptRoot/../../build-extractors/bin/Release/dbc/enUS/SpellEffect.db2",
    [uint32[]]$SpellIDs = @(219615,223763,219912,227058,215387,215598,215607,230503,231511),
    [string]$OutputPath
)
$ErrorActionPreference='Stop'
$b=[IO.File]::ReadAllBytes($Db2Path)
function U32([int]$p) { [BitConverter]::ToUInt32($b,$p) }
if ($b.Length -lt 84 -or [Text.Encoding]::ASCII.GetString($b,0,4) -ne 'WDC1' -or
    (U32 24).ToString('X8') -ne '3244098B' -or
    [BitConverter]::ToUInt16($b,44) -ne 16 -or (U32 8) -ne 29 -or
    (U32 64) -ne 0 -or (U32 40) -ne 0) { throw 'Unsupported SpellEffect layout' }
$count=U32 4; $size=U32 12; $start=84+4*29
$meta=$start+$count*$size+(U32 16)
$pallet=$meta+(U32 68); $common=$pallet+(U32 76); $relation=$common+(U32 72)
if ($relation+(U32 80) -ne $b.Length -or (U32 68) -ne 29*24) { throw 'Invalid file sections' }
$relations=U32 $relation
if (12+8*$relations -ne (U32 80)) { throw 'Unexpected relationship layout' }
$selected=@{}
for($p=$relation+12;$p -lt $b.Length;$p+=8) {
    $spell=U32 $p; $index=U32 ($p+4)
    if($spell -in $SpellIDs) {
        if($index -ge $count) { throw 'Invalid record index' }
        $selected[$index]=$spell
    }
}
$names=@{0='ID';1='Effect';2='EffectBasePoints';3='EffectIndex';4='EffectAura';5='DifficultyID';16='EffectTriggerSpell';26='EffectMiscValues';28='ImplicitTargets'}
$columns=@{}
for($f=0;$f -lt 29;$f++) {
    $p=$meta+24*$f; $type=U32 ($p+8); $extra=U32 ($p+4)
    $col=@{Bit=[int][BitConverter]::ToUInt16($b,$p); Width=[int][BitConverter]::ToUInt16($b,$p+2);Type=$type;Default=(U32 ($p+12));ArraySize=(U32 ($p+20));Pallet=$pallet;Common=@{}}
    if($type -eq 2) {
        if($names.ContainsKey($f)) {
            for($c=$common;$c -lt $common+$extra;$c+=8) { $col.Common[(U32 $c)]=U32 ($c+4) }
        }
        $common+=$extra
    } elseif($type -in 3,4) { $pallet+=$extra }
    if($names.ContainsKey($f)) { $columns[$f]=$col }
}
if($common -ne $relation -or $pallet -ne $meta+(U32 68)+(U32 76)) { throw 'Storage size mismatch' }
$rows=foreach($index in $selected.Keys) {
    $id=U32 ($start+$index*$size)
    $row=[ordered]@{SpellID=$selected[$index]}
    foreach($f in @($names.Keys | Sort-Object)) {
        $col=$columns[$f]; [uint64]$value=0
        if($col.Bit+$col.Width -gt 8*$size) { throw 'Column outside record' }
        if($col.Type -eq 2) {
            $value=$col.Default
            if($col.Common.ContainsKey($id)) { $value=$col.Common[$id] }
        } else {
            for($k=0;$k -lt $col.Width;$k++) {
                $bit=$col.Bit+$k
                if($b[$start+$index*$size+[int][Math]::Floor($bit/8)] -band (1 -shl ($bit%8))) { $value=$value -bor ([uint64]1 -shl $k) }
            }
            if($col.Type -eq 3) { $value=U32 ($col.Pallet+4*$value) }
            elseif($col.Type -eq 4) {
                $values=@(for($a=0;$a -lt $col.ArraySize;$a++) { U32 ($col.Pallet+4*($value*$col.ArraySize+$a)) })
                $row[$names[$f]]=$values; continue
            } elseif($col.Type -notin 0,1) { throw 'Unsupported field compression' }
        }
        $row[$names[$f]]=$value
    }
    [pscustomobject]$row
}
$result=[ordered]@{Source='Local SpellEffect.db2; selected raw unsigned fields only, not all spell data';SHA256=(Get-FileHash $Db2Path -Algorithm SHA256).Hash;RequestedSpellIDs=$SpellIDs;Effects=@($rows | Sort-Object SpellID,EffectIndex)}
$json=$result | ConvertTo-Json -Depth 6
if($OutputPath) {
    [IO.File]::WriteAllText($ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputPath),$json,[Text.UTF8Encoding]::new($false))
    "Exported $($result.Effects.Count) effects."
} else { $json }
