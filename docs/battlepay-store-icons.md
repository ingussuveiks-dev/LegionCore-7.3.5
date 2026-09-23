# Legion 7.3.5 store icons

The loose client overlay discussed below is disabled. Its `overrideArchive=1`
setting caused character-selection model loading failures and WOW51900322
disconnects, including with the StoreUI folder absent. Restore
`overrideArchive=0` with WoW closed; see `client-patches/README.md`.

The first optional uint32 in ProductDisplayInfo is the texture FileDataID.
The old name CreatureDisplayInfoID was misleading: sending a creature ID
there or omitting the field does not provide an item icon. Creature models
belong in the display's Visuals array, alongside their model scene IDs.

Verified against the local 7.3.5.26972 x64 client:

- Shared display builder at preferred VA 0x140A43700 copies the first display
  optional into shared-data offset 0x44 (0x140A43BED through 0x140A43C1E).
- The Lua serializer at 0x1405A4580 exposes offset 0x44 as `texture`.
- The same builder copies native product offset 0x108 into shared offset
  0x58, exposed as `itemID`. The server historically called that product
  wire field `Flags`; it is separate from display flags.
- StoreProductCard_ShowIcon reads `sharedData.texture`, renders it at 64x64,
  and uses `sharedData.itemID` for the item tooltip. It does not derive the
  icon from the deliverables list.

The server resolves each catalog icon from Item.IconFileDataID, falling back
to ItemModifiedAppearance / ItemAppearance.DefaultIconFileDataID. All catalog
display layers receive product context, including shop entry overrides.
Bundles use the first item's icon; multi-item bundles do not advertise a
single-item tooltip. Level-90 and level-100 services use their respective
CharacterServiceInfo icons (614740 and 1033987).

Single-item tooltips are advertised only after the player enters the world.
At character select, GlueXML provides `GlueTooltip` but not `GameTooltip`, and
the stock 7.3.5 Store UI unconditionally indexes `GameTooltip` when `itemID` is
present. Icons and model previews remain available in both environments.

Client acceptance: after reconnecting, check Bags, Weapons, Armor, Toys,
Reinforcements, Gold & Currency, Heirlooms, Raids and the boost card. Check
single-item tooltips and verify that mount/pet models still render. These
item cards show the actual purchased item/token icon, not a 3D preview of
the equipment a token may grant.

The stock 7.3.5 `StoreProductCard_ShowIcon` function reanchors every regular
texture to the top center. That separates the texture from the circular border
on the three-product Featured layout and also ignores the `SplashPrimary`
icon's declared top-left position. This cannot be corrected by a BattlePay
packet field. Install `client-patches/Install-StoreFeaturedIconFix.ps1` to
overlay the affected 7.3.5 UI function for both client architectures. The
patched regular product icons are anchored directly to their `IconBorder`
centres and increased from 64x64 to 68x68 inside the stock 80x81 border.

The complete loose `Blizzard_StoreUI` package, including its TOC, must be
present. A standalone `Blizzard_StoreUISecure.lua` replacement is ignored by
the character-selection GlueXML loader, which otherwise selects the complete
built-in package from CASC.
