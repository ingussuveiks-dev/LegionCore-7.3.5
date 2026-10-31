"""Verify new NPC lifetime alongside retained actor/player/native consumers."""
from pathlib import Path
import runpy
from conversation_npc_lifetime_pe import verify

previous = runpy.run_path(str(Path(__file__).with_name('verify_hruthnir_conversation_release.py')))
native = dict(previous['native'], instructions=previous['previous']['instructions'])
verify(native)
