# Legion 7.3.5 client UI patches

## Featured store icon alignment

The stock 7.3.5 Store UI gives `SplashSecondary` cards two conflicting
anchors: the circular icon border is at the left side of the card, while
`StoreProductCard_ShowIcon` moves the actual icon to the card center. The
same function also lacks the icon anchor declared by `SplashPrimary`.

`Install-StoreFeaturedIconFix.ps1` installs the stock 7.3.5
`Blizzard_StoreUI` package as a loose client overlay and patches
`Blizzard_StoreUISecure.lua`. The package TOC is required because the
character-selection GlueXML otherwise loads the complete built-in StoreUI
from CASC and ignores a standalone loose Lua file. Product icons are anchored
directly to the circular border and rendered at 68x68 inside its stock 80x81
area instead of using the undersized and misplaced stock layout.

Run from the repository root:

```powershell
.\client-patches\Install-StoreFeaturedIconFix.ps1 -ClientPath 'D:\wow\Legion7.3.5'
```

The installer can use a previously exported official 7.3.5 UI source file:

```powershell
.\client-patches\Install-StoreFeaturedIconFix.ps1 `
    -ClientPath 'D:\wow\Legion7.3.5' `
    -SourceFile 'C:\path\to\Blizzard_StoreUISecure.lua'
```

Without `-SourceFile`, it downloads the tagged 7.3.5 file from the Gethe
UI-source mirror. It enables `overrideArchive` and saves the original
`Config.wtf` once as `Config.wtf.store-icon-fix.bak`. The game must be fully
restarted after installation. Both executables share the same Interface and
WTF directories, so the patch covers 32-bit and 64-bit clients.
