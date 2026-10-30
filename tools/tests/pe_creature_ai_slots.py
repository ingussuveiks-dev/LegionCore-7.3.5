"""Find CreatureAI callback slots from the actual linked SmartAI base vtable."""
import re
import struct


def callback_slot(native, callback):
    text = native['args'].release.joinpath('worldserver.map').read_text(errors='replace')
    addresses = re.findall(r'\s+\w+:\w+\s+\?\?_7SmartAI@@6B@\s+([0-9a-fA-F]{16})', text)
    assert len(addresses) == 1, 'Expected exactly one linked SmartAI base vtable'
    slots = struct.unpack_from('<80Q', native['binary'], native['offset'](int(addresses[0], 16)))
    expected = native['symbol'](callback)
    matches = [i for i, raw in enumerate(slots) if raw == expected or
               raw in native['symbols'].values() and native['resolve'](raw) == expected]
    assert len(matches) == 1, ('Native SmartAI callback is absent or ambiguous', callback, matches)
    return matches[0]
