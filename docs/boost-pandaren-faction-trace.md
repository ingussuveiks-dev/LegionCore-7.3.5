# Pandaren boost faction: confirmed packet decoding mismatch

Date: 2026-09-22. Initial investigation was read-only. The subsequent user-requested
fix now uses DecodeBoostChoice in the assignment packet reader; see
`battlepay-boost-wire-format.md` and `tools/tests/BattlePayBoostChoiceTest.cpp`.

## Observed failure

The diagnostic log at16:45:53 rejects Yim (guid4, neutral Pandaren race24,
Monk class10, level60) solely for `invalid_faction_choice`. It reports
specialization269, choice0, resolved product110 and one unused credit.

## Client evidence

Examined the running `D:/wow/Legion7.3.5/Wow-64_Patched.exe` code using
OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ) and ReadProcessMemory.
No writes, injection, breakpoints or client actions were used. On-disk code
contains protected regions; the executing code was read for disassembly.

Executable SHA256:
`BE484C9A3BD8E5DE173E9A8E185A9B6636406D69A9122987F6F2C1D512B2BEC5`.
Addresses below are RVAs, independent of ASLR. The analysis helper is
`tools/research/Read-BoostClientCode.ps1`, using Iced1.21.0 from its official
NuGet package, cached under `%TEMP%/boost-readonly-disassembly`.

- Lua source reference: CharacterServices.lua, AssignUpgradeDistribution
  receives guid, faction, specID, classID, boostType. The cached UI checkout
  is7.3.5.26124, so native evidence from the actual running client is decisive.
- Registration at RVA56E791 points AssignUpgradeDistribution to56E8F0.
- At56E9E8 the binding reads Lua argument2. At56E9EF it subtracts double1.0
  (constant RVA12AA3A8), then stores the faction enum at rsp+30h.
- At56EB39..56EB5A it forwards the parsed arguments to4CC600: faction in edx,
  spec in r8d, class in r9d, boost type on the stack.
- At4CC6A6 it shifts the zero-based faction left24 bits; at4CC6AC it ORs the
  specialization into the same uint32 and passes it toD6BE80.
- AtD6BF6C the packed uint32 is stored at packet object offset40h. Product0,
  distribution and GUID occupy offsets20h,28h and30h respectively.
- Constructor4D0550 uses vtable RVA12F8ED8. Its serializer4D7630 writes
  opcode36C0, uint32 product, uint64 distribution, packed GUID, then a **single
  uint32** from object offset40h (4D7670..4D7676).

Thus the wire value is `(zeroBasedFaction << 24) | specializationId`, with
Horde=0 and Alliance=1. The existing service-level faction convention is
different: Horde=1, Alliance=2.

## Why the server rejects it

`DistributionAssignToTarget::Read()` in BattlePayPackets.cpp reads the final
uint32 as `uint16 SpecializationID` followed by `uint16 ChoiceID`.

For specialization269:

| Selection | Wire uint32 | Little-endian bytes | Existing server values |
| --- | --- | --- | --- |
| Horde | 0x0000010D | 0D 01 00 00 | spec269, choice0 |
| Alliance | 0x0100010D | 0D 01 00 01 | spec269, choice256 |

The Horde result exactly explains the observed log. Alliance is derived from
the native encoding; no additional player boost was attempted to test it.

## Fix identified during diagnosis and subsequently implemented

Read the final field as uint32, extract the upper-byte faction and lower24-bit
specialization, validate both without truncation, and normalize the faction
to the existing service convention1/2. Retain ownership, entitlement, product,
level and class checks; reject invalid faction bytes and oversized spec IDs.
Do not simply accept arbitrary choice0 for neutral Pandaren or choose a
default faction. Add Horde/Alliance/invalid-value decoding tests before deployment.
