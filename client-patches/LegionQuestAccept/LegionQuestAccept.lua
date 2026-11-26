local pendingQuest

QuestFrameAcceptButton:HookScript("PreClick", function()
    pendingQuest = nil
    if not QuestFrameDetailPanel:IsVisible() or QuestFrame.autoQuest then
        return
    end

    local giver = UnitGUID("questnpc")
    local questID = GetQuestID()
    if giver and giver:match("^Creature%-") and questID and questID > 0
        and (GetQuestLogIndexByID(questID) or 0) == 0 then
        pendingQuest = questID
    end
end)

-- The native greeting, next offer, reward, and manual close all hide this panel.
QuestFrameDetailPanel:HookScript("OnHide", function()
    pendingQuest = nil
end)

local listener = CreateFrame("Frame")
listener:RegisterEvent("QUEST_ACCEPTED")
listener:SetScript("OnEvent", function(_, _, questLogIndex, questID)
    if not pendingQuest or questID ~= pendingQuest then
        return
    end

    pendingQuest = nil
    if questLogIndex and questLogIndex > 0 and QuestFrameDetailPanel:IsVisible() then
        HideUIPanel(QuestFrame)
    end
end)
