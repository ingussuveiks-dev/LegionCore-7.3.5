[CmdletBinding()]
param(
    [Parameter()]
    [string] $ClientPath = 'D:\wow\Legion7.3.5',

    [Parameter()]
    [string] $SourceFile
)

$ErrorActionPreference = 'Stop'

$storeRelativePath = 'Interface\AddOns\Blizzard_StoreUI\Blizzard_StoreUISecure.lua'
$targetFile = Join-Path $ClientPath $storeRelativePath
$targetDirectory = Split-Path -Parent $targetFile
$configFile = Join-Path $ClientPath 'WTF\Config.wtf'
$sourceUrl = 'https://raw.githubusercontent.com/Gethe/wow-ui-source/7.3.5/AddOns/Blizzard_StoreUI/Blizzard_StoreUISecure.lua'
$utf8WithoutBom = New-Object System.Text.UTF8Encoding($false)

if (-not (Test-Path -LiteralPath $ClientPath -PathType Container)) {
    throw "WoW client directory does not exist: $ClientPath"
}

New-Item -ItemType Directory -Path $targetDirectory -Force | Out-Null

if (-not (Test-Path -LiteralPath $targetFile -PathType Leaf)) {
    if ($SourceFile) {
        if (-not (Test-Path -LiteralPath $SourceFile -PathType Leaf)) {
            throw "Source Store UI file does not exist: $SourceFile"
        }

        $storeSource = [System.IO.File]::ReadAllText((Resolve-Path -LiteralPath $SourceFile))
    }
    else {
        $storeSource = (Invoke-WebRequest -UseBasicParsing -Uri $sourceUrl).Content
    }

    [System.IO.File]::WriteAllText($targetFile, $storeSource, $utf8WithoutBom)
}

$originalAnchor = @'
	self.Icon:ClearAllPoints();
	self.Icon:SetPoint("CENTER", self, "TOP", 0, -69);
	if (not overrideTexture) then
		if (self == StoreFrame.SplashSingle) then
			self.Icon:ClearAllPoints();
			self.Icon:SetPoint("TOPLEFT", 88, -99);
		end
'@

$fixedAnchor = @'
	self.Icon:ClearAllPoints();
	if (not overrideTexture) then
		if (self == StoreFrame.SplashSingle) then
			self.Icon:SetPoint("TOPLEFT", 88, -99);
		elseif (self == StoreFrame.SplashPrimary) then
			self.Icon:SetPoint("TOPLEFT", 88, -76);
		elseif (self == StoreFrame.SplashSecondary1 or self == StoreFrame.SplashSecondary2) then
			self.Icon:SetPoint("CENTER", self, "CENTER", -70, 6);
		else
			self.Icon:SetPoint("CENTER", self, "TOP", 0, -69);
		end
'@

$originalIconSize = "`t`tself.Icon:SetSize(64, 64);"
$fixedIconSize = "`t`tself.Icon:SetSize(68, 68);"

$storeText = [System.IO.File]::ReadAllText($targetFile)
$normalizedStoreText = $storeText.Replace("`r`n", "`n")
if (-not $normalizedStoreText.Contains($fixedAnchor)) {
    if (-not $normalizedStoreText.Contains($originalAnchor)) {
        throw "The Store UI file is not the expected 7.3.5 version; no changes were made."
    }

    $normalizedStoreText = $normalizedStoreText.Replace($originalAnchor, $fixedAnchor)
}

if ($normalizedStoreText.Contains($originalIconSize)) {
    $normalizedStoreText = $normalizedStoreText.Replace($originalIconSize, $fixedIconSize)
}
elseif (-not $normalizedStoreText.Contains($fixedIconSize)) {
    throw "The Store UI icon size statement is not the expected 7.3.5 version; no changes were made."
}

[System.IO.File]::WriteAllText($targetFile, $normalizedStoreText, $utf8WithoutBom)

if (-not (Test-Path -LiteralPath $configFile -PathType Leaf)) {
    throw "WoW Config.wtf does not exist: $configFile"
}

$configBackup = "$configFile.store-icon-fix.bak"
if (-not (Test-Path -LiteralPath $configBackup -PathType Leaf)) {
    Copy-Item -LiteralPath $configFile -Destination $configBackup
}

$configText = [System.IO.File]::ReadAllText($configFile)
if ($configText -match '(?m)^SET overrideArchive\s+"[^"]*"\s*$') {
    $configText = [System.Text.RegularExpressions.Regex]::Replace(
        $configText,
        '(?m)^SET overrideArchive\s+"[^"]*"\s*$',
        'SET overrideArchive "1"')
}
else {
    if ($configText.Length -gt 0 -and -not $configText.EndsWith("`n")) {
        $configText += "`r`n"
    }

    $configText += "SET overrideArchive `"1`"`r`n"
}

[System.IO.File]::WriteAllText($configFile, $configText, $utf8WithoutBom)

Write-Host "Installed the Legion 7.3.5 Featured-store icon alignment fix."
Write-Host "Client UI: $targetFile"
Write-Host "Restart WoW completely before testing either the 32-bit or 64-bit executable."
