"""Structural LINK admission matching SmartScript::ProcessAction's LINK tail.

This operates on exported rows, not the live loader's difficulty/condition set.
It proves neither that a source event fires nor what a missing action should do.
"""


def invalid_links(group, *, owner_events=None):
    # Native FindLinkedEvent returns the first matching event ID. Database rows
    # normally have unique IDs; preserve that behavior for malformed fixtures.
    targets = {}
    # Timed-list rows execute on their recipient SmartScript. FindLinkedEvent
    # searches that owner's mEvents, not mTimedActionList (source_type 9).
    for row in group if owner_events is None else owner_events:
        targets.setdefault(row['id'], row)
    result = []
    for row in group:
        link = row['link']
        if not link or link == row['id']:
            continue
        target = targets.get(link)
        reason = ('missing_target' if target is None else
                  'empty_target_action' if not target['action_type'] else
                  'invalid_target_event' if target['event_type'] != 61 else None)
        if reason:
            finding = dict(entry=row['entryorguid'], event=row['id'], link=link, reason=reason)
            if target is not None:
                finding.update(target_event_type=target['event_type'], target_action_type=target['action_type'])
            result.append(finding)
    return result
