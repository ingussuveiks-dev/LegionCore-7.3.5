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
$sourceBaseUrl = 'https://raw.githubusercontent.com/Gethe/wow-ui-source/7.3.5/AddOns/Blizzard_StoreUI'
$sourceUrl = "$sourceBaseUrl/Blizzard_StoreUISecure.lua"
$utf8WithoutBom = New-Object System.Text.UTF8Encoding($false)

if (-not (Test-Path -LiteralPath $ClientPath -PathType Container)) {
    throw "WoW client directory does not exist: $ClientPath"
}

New-Item -ItemType Directory -Path $targetDirectory -Force | Out-Null

# GlueXML selects the complete built-in StoreUI package from CASC when the
# loose override has no TOC. Install the stock companion files as well so the
# patched package is selected both before and after entering the world.
$storePackageFiles = @(
    'Blizzard_SimpleCheckout.lua',
    'Blizzard_SimpleCheckout.xml',
    'Blizzard_StoreUI.toc',
    'Blizzard_StoreUI.xml',
    'Blizzard_StoreUIInbound.lua',
    'Blizzard_StoreUIInsecure.lua',
    'Blizzard_StoreUIInsecure.xml',
    'Blizzard_StoreUIOutbound.lua',
    'Blizzard_StoreUIPatchwerk.xml',
    'Localization.lua'
)

foreach ($storePackageFile in $storePackageFiles) {
    $packageTarget = Join-Path $targetDirectory $storePackageFile
    if (-not (Test-Path -LiteralPath $packageTarget -PathType Leaf)) {
        $packageSource = (Invoke-WebRequest -UseBasicParsing -Uri "$sourceBaseUrl/$storePackageFile").Content
        [System.IO.File]::WriteAllText($packageTarget, $packageSource, $utf8WithoutBom)
    }
}

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

$fixedAnchorV1 = @'
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

$fixedAnchor = @'
	self.Icon:ClearAllPoints();
	if (not overrideTexture) then
		if (self == StoreFrame.SplashSingle) then
			self.Icon:SetPoint("TOPLEFT", 88, -99);
		elseif (self == StoreFrame.SplashPrimary) then
			self.Icon:SetPoint("TOPLEFT", 88, -76);
		else
			self.Icon:SetPoint("CENTER", self.IconBorder, "CENTER", 0, 0);
		end
'@

$originalIconSize = "`t`tself.Icon:SetSize(64, 64);"
$fixedIconSize = "`t`tself.Icon:SetSize(68, 68);"

$storeText = [System.IO.File]::ReadAllText($targetFile)
$normalizedStoreText = $storeText.Replace("`r`n", "`n")
if (-not $normalizedStoreText.Contains($fixedAnchor)) {
    if ($normalizedStoreText.Contains($fixedAnchorV1)) {
        $normalizedStoreText = $normalizedStoreText.Replace($fixedAnchorV1, $fixedAnchor)
    }
    elseif ($normalizedStoreText.Contains($originalAnchor)) {
        $normalizedStoreText = $normalizedStoreText.Replace($originalAnchor, $fixedAnchor)
    }
    else {
        throw "The Store UI file is not the expected 7.3.5 version; no changes were made."
    }
}

if ($normalizedStoreText.Contains($originalIconSize)) {
    $normalizedStoreText = $normalizedStoreText.Replace($originalIconSize, $fixedIconSize)
}
elseif (-not $normalizedStoreText.Contains($fixedIconSize)) {
    throw "The Store UI icon size statement is not the expected 7.3.5 version; no changes were made."
}

# A disconnect or catalog refresh can invalidate a card before its mouse
# callbacks run. Never dereference an entry from the previous catalog.
$guardedCallbacks = @(
    'StoreProductCard_OnEnter',
    'StoreProductCard_OnClick',
    'StoreProductCardMagnifyingGlass_OnClick',
    'StoreProductCardItem_OnEnter'
)
foreach ($callback in $guardedCallbacks) {
    $pattern = '(?s)(function ' + [regex]::Escape($callback) + '\([^\r\n]*\)\n.*?local entryInfo = C_StoreSecure\.GetEntryInfo\([^\r\n]*\);\n)'
    $match = [regex]::Match($normalizedStoreText, $pattern)
    if (-not $match.Success) {
        throw "Cannot find Store UI callback: $callback"
    }
    $guard = "`tif (not entryInfo or not entryInfo.sharedData) then`n`t`treturn;`n`tend`n"
    if (-not $normalizedStoreText.Substring($match.Index + $match.Length).StartsWith($guard)) {
        $normalizedStoreText = $normalizedStoreText.Insert($match.Index + $match.Length, $guard)
    }
}
$normalizedStoreText = $normalizedStoreText.Replace(
    'if ( entryInfo.displayID ) then',
    'if ( entryInfo and entryInfo.displayID ) then')
$normalizedStoreText = $normalizedStoreText.Replace(
    'return entryInfo and #entryInfo.sharedData.cards > 0;',
    'return entryInfo and entryInfo.sharedData and entryInfo.sharedData.cards and #entryInfo.sharedData.cards > 0;')

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
Write-Host "Client UI package: $targetDirectory"
Write-Host "Restart WoW completely before testing either the 32-bit or 64-bit executable."
