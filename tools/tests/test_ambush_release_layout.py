"""Counterfactual binary snippets must fail while old/new valid layouts pass."""
import struct
import unittest
from capstone import Cs, CS_ARCH_X86, CS_MODE_64
from ambush_release_layout import check_provenance_layout


def snippets(field=0x258, constructor=None, pre_read=None, pre_write=None, size=None, initial=0):
    constructor = field if constructor is None else constructor
    pre_read = field if pre_read is None else pre_read
    pre_write = field if pre_write is None else pre_write
    size = field + 8 if size is None else size
    dword = lambda n: struct.pack('<I', n)
    decoder = Cs(CS_ARCH_X86, CS_MODE_64); decoder.detail = True
    factory = b'\xb9' + dword(size) + b'\xe8\x00\x00\x00\x00\x48\x89\xc7\xc7\x87' + dword(constructor) + dword(initial)
    post = b'\x4c\x8d\x89' + dword(field)
    pre = b'\x83\xb9' + dword(pre_read) + b'\x00\x89\x83' + dword(pre_write)
    return [list(decoder.disasm(code, 0x140000000)) for code in (factory, post, pre)]


class LayoutTests(unittest.TestCase):
    def test_original_and_current_layouts(self):
        for field in (0x200, 0x258):
            with self.subTest(field=field): self.assertEqual(check_provenance_layout(*snippets(field)), field)

    def test_real_mismatches_remain_failures(self):
        for change in ({'constructor':0x200}, {'pre_read':0x200}, {'pre_write':0x200}, {'size':0x258}, {'initial':1}):
            with self.subTest(change=change), self.assertRaises(AssertionError): check_provenance_layout(*snippets(**change))

    def test_missing_initialization_and_ambiguous_post_field(self):
        factory, post, pre = snippets()
        with self.assertRaises(AssertionError): check_provenance_layout(factory[:3], post, pre)
        with self.assertRaises(AssertionError): check_provenance_layout(factory, post + post, pre)


if __name__ == '__main__': unittest.main()
