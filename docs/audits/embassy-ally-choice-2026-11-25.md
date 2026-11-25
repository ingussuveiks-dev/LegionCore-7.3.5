# Legion embassy ally choice credit

On the existing live server the Horde quest `50242` recorded both embassy conversations and left `65016` at zero after the player selected Nightborne. The account had neither the Highmountain (`12445`) nor the Nightborne (`12446`) unlock-requirements achievement. The response rows already had those achievement conditions, but embassy choices skipped response validation when displaying and handling the choice.

The build 26972 spell and criteria records explain a separate failure for a player who meets the requirements. The four original choice spells use effect 16 to finish hidden quest markers. The marker templates lack the generic Tracking flag and have no separate quest-log entry. The old effect fell through without recording a marker. The normal hidden-marker storage function also does not send the type-27 quest-completion criterion update.

| Choice | Response | Spell | Hidden marker |
| --- | ---: | ---: | ---: |
| Highmountain Tauren | 752 | 247978 | 47957 |
| Nightborne | 753 | 247980 | 47958 |
| Void Elves | 783 | 251724 | 48602 |
| Lightforged Draenei | 784 | 251725 | 48603 |

The existing reward spell must hit the player and match the branch before recording the marker and notifying the native quest criteria. Both embassy choices now recheck their existing achievement conditions, faction, interaction with the questgiver and active quest at display, response and hit. First selection requires both recorded conversations. Second selection requires the previous ally and only offers the remaining eligible response. The second objective trees `65024` and `65104` need both markers before a criterion can be credited, which prevents login recovery from counting the previous ally as a second selection. The standard delayed criterion and quest update paths remain in use. No achievement, allied race or scenario reward is granted by the choice handler.

Evidence covers the native DB2 spell and criteria rows, live choice and condition data, and source dispatch review. The active client workflow uses the Release build and final-link check, with eligible client selection, turn-in, second ally and subsequent recruitment still pending. No scripted gameplay tests, isolated startup tests or character-data edits were performed for this active flow. [Original Legion recruitment requirements](https://worldofwarcraft.blizzard.com/en-us/news/21508135/building-reputation-with-allied-races)
