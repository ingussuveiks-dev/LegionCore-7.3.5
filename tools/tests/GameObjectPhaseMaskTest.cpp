#include "FieldValueConverters.h"
#include <array>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

// Use the production numeric converters. Turn their fatal truncation reporter
// into a test exception so the old UInt16 loader can be checked as a control.
BaseDatabaseResultValueConverter::BaseDatabaseResultValueConverter() = default;
BaseDatabaseResultValueConverter::~BaseDatabaseResultValueConverter() = default;
void BaseDatabaseResultValueConverter::LogTruncation(char const*, QueryResultFieldMetadata const*)
{
    throw std::overflow_error("database field truncation");
}

struct InputField
{
    BaseDatabaseResultValueConverter const* converter = nullptr;
    char const* value = nullptr;
    uint32 length = 0;
    uint16 GetUInt16() const { return converter->GetUInt16(value,length,nullptr); }
    uint32 GetUInt32() const { return converter->GetUInt32(value,length,nullptr); }
};

uint32 LoadMask(InputField const* fields)
{
    struct { uint32 phaseMask = 1; } data;
    // Extracted from ObjectMgr::LoadGameobjects, not a copied implementation.
#include "GameObjectPhaseMask.inc"
    return data.phaseMask;
}

int main()
{
    PrimitiveResultValueConverter<uint32,FromStringToDatabaseTypeConverter> text;
    PrimitiveResultValueConverter<uint32,FromBinaryToDatabaseTypeConverter> binary;
    std::array<InputField,19> fields{};
    std::array<uint32,8> const masks{{0u,1u,65535u,65536u,65537u,0x80000000u,0x80000001u,0xFFFFFFFFu}};
    for (uint32 mask : masks)
    {
        std::string encoded = std::to_string(mask);
        for (bool prepared : {false,true})
        {
            fields[18] = {prepared ? static_cast<BaseDatabaseResultValueConverter*>(&binary) : &text,
                prepared ? reinterpret_cast<char const*>(&mask) : encoded.data(),
                prepared ? uint32(sizeof(mask)) : uint32(encoded.size())};
            assert(LoadMask(fields.data()) == mask);
            // Verify that the real converter would have rejected the old getter.
            bool truncated = false;
            try { (void)fields[18].GetUInt16(); }
            catch (std::overflow_error const&) { truncated = true; }
            assert(truncated == (mask > 65535u));
        }
    }
    std::cout << "PASS: production loader preserves all 32 phase bits through text/binary converters; old UInt16 getter rejects high masks.\n";
}
