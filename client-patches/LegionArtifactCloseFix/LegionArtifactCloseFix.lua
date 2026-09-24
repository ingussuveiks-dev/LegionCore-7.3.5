local function RepairArtifactCloseButton()
    local artifactFrame = ArtifactFrame
    if not artifactFrame or not artifactFrame.CloseButton then
        return
    end

    local button = artifactFrame.CloseButton
    local overlay = artifactFrame.VisitForgeOverlay
    if overlay then
        -- In the stock 7.3.5 UI the overlay is level 3500 and the close
        -- button is level 2500, so an untouched artifact traps mouse clicks.
        button:SetFrameStrata(overlay:GetFrameStrata())
        button:SetFrameLevel(overlay:GetFrameLevel() + 1)
    end
    button:SetHitRectInsets(0, 0, 0, 0)
    button:EnableMouse(true)
    button:Enable()
    button:Show()
end

local function InstallArtifactCloseFix()
    if not ArtifactFrame or ArtifactFrame.LegionCloseFixInstalled then
        return
    end

    ArtifactFrame.LegionCloseFixInstalled = true
    ArtifactFrame:HookScript("OnShow", RepairArtifactCloseButton)
    RepairArtifactCloseButton()
end

local loader = CreateFrame("Frame")
loader:RegisterEvent("ADDON_LOADED")
loader:SetScript("OnEvent", function(self, event, addonName)
    if addonName == "Blizzard_ArtifactUI" then
        InstallArtifactCloseFix()
        self:UnregisterEvent("ADDON_LOADED")
    end
end)

if IsAddOnLoaded("Blizzard_ArtifactUI") then
    InstallArtifactCloseFix()
    loader:UnregisterEvent("ADDON_LOADED")
end
