"""Loopback-only pre-authentication probe; sends no account/session credentials."""
import argparse
from pathlib import Path
import re
import socket
import struct
import time

parser = argparse.ArgumentParser()
parser.add_argument("--count", type=int, default=8)
args = parser.parse_args()
if not 1 <= args.count <= 32:
    raise SystemExit("count must be 1..32")
repo = Path(__file__).resolve().parents[2]
opcodes = (repo / "src/server/game/Server/Protocol/Opcodes.h").read_text(encoding="utf-8-sig")
opcode = int(re.search(r"SMSG_AUTH_CHALLENGE\s*=\s*(0x[0-9A-Fa-f]+)", opcodes)[1], 16)


def receive(connection, count):
    data = b""
    while len(data) < count:
        part = connection.recv(count - len(data))
        if not part:
            raise RuntimeError("peer closed an unfinished initializer/challenge")
        data += part
    return data


for iteration in range(args.count):
    # Exercise the instance listener first on a fresh validation server.
    for port in (8086, 8085):
        begin = time.monotonic()
        with socket.create_connection(("127.0.0.1", port), timeout=3) as connection:
            connection.settimeout(3)
            assert receive(connection, 48) == b"WORLD OF WARCRAFT CONNECTION - SERVER TO CLIENT\n"
            connection.sendall(b"WORLD OF WARCRAFT CONNECTION - CLIENT TO SERVER\n")
            header = receive(connection, 6)
            size, actual_opcode = struct.unpack("<IH", header)
            assert size == 51 and actual_opcode == opcode
            assert len(receive(connection, size - 2)) == 49
        print(f"PASS: loopback {port}, greeting 48 bytes and native auth challenge 55 bytes, {(time.monotonic()-begin)*1000:.1f}ms")
print("Initializer transport only: no account authentication or real WoW-client login was tested.")
