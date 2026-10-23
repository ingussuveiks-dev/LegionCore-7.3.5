"""Compare a locally fetched, pinned Legion reference without logging key data."""
import argparse
import hashlib
import json
from pathlib import Path
import re

parser = argparse.ArgumentParser()
parser.add_argument("reference", type=Path)
args = parser.parse_args()
repo = Path(__file__).resolve().parents[2]


def text(path):
    return path.read_text(encoding="utf-8-sig")


def block(source, marker):
    start = source.index(marker)
    begin = source.index("{", start)
    depth = 1
    end = begin + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[begin:end]


def normalize(source):
    return re.sub(r"\s+", "", source)


local = text(repo / "src/server/game/Server/WorldSocket.cpp")
reference = text(args.reference / "WorldSocket.cpp")
checks = {}
for name in ("ServerConnectionInitialize", "ClientConnectionInitialize"):
    pattern = rf'{name}\("([^"]+)"\)'
    a, b = re.search(pattern, local)[1], re.search(pattern, reference)[1]
    checks[name] = a == b
    checks[name + "_wire_bytes"] = len(a.encode()) + 1
for name in ("AuthCheckSeed", "SessionKeySeed", "ContinuedSessionSeed"):
    pattern = rf"{name}\[16\]\s*=\s*\{{([^}}]+)\}}"
    checks[name] = normalize(re.search(pattern, local)[1]) == normalize(re.search(pattern, reference)[1])
for name in ("InitializeHandler", "CheckIpCallback"):
    body = block(reference, "void WorldSocket::" + name + "(")
    checks[name + "_reference_present"] = bool(body)
def active_start(source):
    return re.sub(r"//[^\n]*|/\*.*?\*/", "", block(source, "void WorldSocket::Start()"), flags=re.S)


checks["reference_async_ip_check"] = "LoginDatabase.AsyncQuery" in active_start(reference)
checks["local_async_ip_check"] = "LoginDatabase.AsyncQuery" in active_start(local)
checks["reference_network_callback_greeting"] = "QueuePacket(std::move(initializer))" in block(reference, "void WorldSocket::CheckIpCallback(")

packets = text(repo / "src/server/game/Server/Packets/AuthenticationPackets.cpp")
ref_packets = text(args.reference / "AuthenticationPackets.cpp")
for name in ("AuthChallenge", "AuthContinuedSession", "ConnectToFailed"):
    method = "Write" if name == "AuthChallenge" else "Read"
    marker = "WorldPackets::Auth::" + name + "::" + method + "()"
    checks[name + "_serialization"] = normalize(block(packets, marker)) == normalize(block(ref_packets, marker))
a = block(packets, "WorldPackets::Auth::ConnectTo::Write()")
b = block(ref_packets, "WorldPackets::Auth::ConnectTo::Write()")
for label, begin, end in (("connect_payload_order", "ByteBuffer payload;", "uint32 rsaSize"),
                          ("connect_envelope_order", "_worldPacket << uint64(Key);", "ConnectToRSA->Encrypt")):
    checks[label] = normalize(a[a.index(begin):a.index(end)]) == normalize(b[b.index(begin):b.index(end)])
# Hash the public upstream protocol key for equality, never print its contents.
key = r'RSAPrivateKey\s*=\s*R"\((.*?)\)"'
checks["connect_protocol_rsa_key"] = hashlib.sha256(re.search(key, packets, re.S)[1].encode()).digest() == hashlib.sha256(re.search(key, ref_packets, re.S)[1].encode()).digest()
for name in ("WherePacketHmac", "PiDigits"):
    pattern = rf"{name}\[[0-9]*\]\s*=\s*\{{([^}}]+)\}}"
    checks[name] = normalize(re.search(pattern, packets)[1]) == normalize(re.search(pattern, ref_packets)[1])
for label, pattern in (("connect_haiku", r'ConnectTo::Haiku\(([^;]+)\);'),
                       ("connect_panama_constant", r'HexStrToByteArray\("([^"]+)", Payload.PanamaKey\)'),
                       ("connect_adler_constant", r'Payload.Adler32\s*=\s*(0x[0-9A-Fa-f]+)')):
    checks[label] = re.search(pattern, packets)[1] == re.search(pattern, ref_packets)[1]
headers = text(repo / "src/server/game/Server/Packets/AuthenticationPackets.h")
ref_headers = text(args.reference / "AuthenticationPackets.h")
checks["connect_attempt_serials"] = normalize(block(headers, "enum class ConnectToSerial")) == normalize(block(ref_headers, "enum class ConnectToSerial"))
checks["connect_xor_constant"] = re.search(r'XorMagic\s*=\s*(0x[0-9A-Fa-f]+)', headers)[1] == re.search(r'XorMagic\s*=\s*(0x[0-9A-Fa-f]+)', ref_headers)[1]
print(json.dumps(checks, indent=2))
required = [name for name in checks if name not in ("local_async_ip_check",) and not name.endswith("_wire_bytes")]
if not all(checks[name] for name in required):
    raise SystemExit("Protocol difference requires review; not a proven cause of a silent initial handshake")
