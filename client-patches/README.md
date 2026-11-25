# Legion 7.3.5 client UI patches

## Artifact window close button

`LegionArtifactCloseFix` is a standalone 7.3.5 addon. The
[stock 7.3.5 artifact UI](https://github.com/Gethe/wow-ui-source/blob/7.3.5/AddOns/Blizzard_ArtifactUI/Blizzard_ArtifactUI.xml)
gives the “Visit your forge” overlay frame level 3500, above the close button
at level 2500. The overlay intercepts its mouse clicks. The addon places the
button one level above the overlay and restores its click area. It does not
change the artifact's powers or the client's archive settings. Install it with:

```powershell
.\client-patches\Install-LegionArtifactCloseFix.ps1 -ClientPath 'D:\wow\Legion7.3.5'
```

Reload the client UI once after installation. A newly purchased artifact with
zero purchased traits displays the stock “Visit your forge” overlay away from
its class-hall artifact forge. The first trait costs 100 Artifact Power and
must be unlocked at the forge; level 110 is not required. Ulthalesh uses the
Cursed Forge of the Nathrezim in the warlock class hall.

## Retired StoreUI override

The Featured store section uses the stock card layout, set by the server.
The previous client overlay enabled `overrideArchive` and reproduced
`WOW51900322` disconnects in character selection. `Model.log` reported
`E_NOT_AVAILABLE` for Undead male HD model 959310. Disabling
`overrideArchive` resolved the disconnect in the user's client check.
[Store icon implementation](../docs/battlepay-store-icons.md)

Clients that used the old overlay can return to the stock UI by closing WoW,
moving `Interface\AddOns\Blizzard_StoreUI` out of `Interface\AddOns`, and
setting `SET overrideArchive "0"` in `WTF\Config.wtf` before restarting.
