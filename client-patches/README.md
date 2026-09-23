# Legion 7.3.5 client UI patches

## StoreUI override disabled

Do not install the loose StoreUI overlay described below. The installer now
stops before changing any files. On the local 7.3.5 client, enabling
`overrideArchive` caused `WOW51900322` even when no shop was opened and after
the StoreUI folder had been removed. `Model.log` reported `E_NOT_AVAILABLE`
for the Undead male HD model (FileDataID 959310); the model and its skin files
were independently readable from the local CASC archive. Disabling
`overrideArchive` stopped the disconnect in the user's character-select and
shop test.

To restore the stock client, completely close WoW, move
`Interface\AddOns\Blizzard_StoreUI` outside `Interface\AddOns` (keep a backup),
and set `SET overrideArchive "0"` in `WTF\Config.wtf`. Restart the client.
Both client executables share this configuration, but the successful user
retest was performed with the 64-bit executable. The stock icon alignment
remains a cosmetic issue; the loose overlay is not a supported fix.

The installation details below are historical reference only.

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

The installer also guards product mouse, click, tooltip, and preview callbacks
against catalog entries disappearing during a refresh or disconnect. This fixes
the `StoreProductCard_OnEnter` nil `entryInfo` error reported at line 3381.
It does not establish or fix the cause of a simultaneous `WOW51900322` streaming
disconnect: the available client logs did not identify a failing asset. Verify
opening the store both at character selection and in-world after a full restart.
