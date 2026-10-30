// Actual DB2FileLoader materialization, generated WDC1 records, no world stubs.
#include "DB2FileLoader.h"
#include "DB2Meta.h"
#include "Common.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

class MemorySource : public DB2FileSource
{
public:
    std::vector<uint8> bytes;
    std::size_t position = 0;
    bool IsOpen() const override { return true; }
    bool Read(void* target, std::size_t length) override
    {
        if (length > bytes.size() - position) return false;
        std::memcpy(target, bytes.data() + position, length); position += length; return true;
    }
    std::size_t GetPosition() const override { return position; }
    std::size_t GetFileSize() const override { return bytes.size(); }
    char const* GetFileName() const override { return "generated sparse strings.db2"; }
    template<class T> void Append(T const& value)
    {
        auto at = bytes.size(); bytes.resize(at + sizeof(value)); std::memcpy(bytes.data() + at, &value, sizeof(value));
    }
    void Append(std::string const& text) { bytes.insert(bytes.end(), text.begin(), text.end()); bytes.push_back(0); }
};
#pragma pack(push, 1)
struct Field { int16 unused; uint16 offset; };
struct Catalog { uint32 offset; uint16 size; };
#pragma pack(pop)
struct Serialized { std::vector<uint8> bytes; };
static MemorySource Make(DB2FileLoadInfo const& info, uint32 locale, std::map<uint32, uint32> const& ids,
                         std::vector<MemorySource> const& physical, std::vector<int16> const& widths)
{
    MemorySource file;
    DB2Header header{};
    header.Signature = 0x31434457; header.RecordCount = uint32(ids.size()); header.FieldCount = info.Meta->FieldCount;
    header.LayoutHash = info.Meta->LayoutHash; header.MinId = ids.empty() ? 1 : ids.begin()->first;
    header.MaxId = ids.empty() ? 1 : ids.rbegin()->first; header.Flags = 1; header.Locale = 1u << locale;
    header.IndexField = int16(info.Meta->IndexField); header.TotalFieldCount = header.FieldCount;
    header.IdTableSize = info.Meta->HasIndexFieldInData() ? 0 : 4 * header.RecordCount;
    file.Append(header);
    for (auto width : widths) file.Append(Field{width, 0});
    assert(widths.size() == header.FieldCount);
    std::vector<Catalog> locations;
    for (auto const& row : physical)
    {
        assert(row.bytes.size() < 65536);
        locations.push_back({uint32(file.bytes.size()), uint16(row.bytes.size())});
        file.bytes.insert(file.bytes.end(), row.bytes.begin(), row.bytes.end());
    }
    header.CatalogDataOffset = uint32(file.bytes.size());
    for (uint32 id = header.MinId; id <= header.MaxId; ++id)
    {
        auto found = ids.find(id); file.Append(found == ids.end() ? Catalog{} : locations.at(found->second));
    }
    if (!info.Meta->HasIndexFieldInData()) for (auto const& id : ids) file.Append(id.first);
    std::memcpy(file.bytes.data(), &header, sizeof(header)); return file;
}
struct Materialized
{
    DB2FileLoader loader;
    uint32 size = 0; char** index = nullptr; char* data = nullptr; std::vector<char*> pools;
    Materialized(MemorySource& source, DB2FileLoadInfo const& info)
    {
        assert(loader.Load(&source, &info)); data = loader.AutoProduceData(size, index, pools); assert(data && index);
    }
    ~Materialized() { delete[] data; delete[] index; for (auto p : pools) delete[] p; }
    void Locale(uint32 locale) { auto block = loader.AutoProduceStrings(index, size, locale); assert(block); pools.push_back(block); }
};
template<class T> static T At(char const* record, std::size_t offset)
{
    T value; std::memcpy(&value, record + offset, sizeof(value)); return value;
}
static char const* Local(Materialized const& table, uint32 id, uint32 locale, std::size_t offset = 4)
{
    return At<LocalizedString*>(table.index[id], offset)->Str[locale];
}
static void Strings(bool plain, bool empty)
{
    static uint8 const arrays[]{1, 1, 1}; DB2Meta meta(-1, 3, 0x12345678, "ssi", arrays, -1);
    DB2FieldMeta fields[]{{false, FT_INT, "ID"}, {false, FT_STRING, "Name"},
                         {false, plain ? FT_STRING_NOT_LOCALIZED : FT_STRING, "Other"}, {false, FT_INT, "Value"}};
    DB2FileLoadInfo info(fields, 4, &meta);
    MemorySource row; row.Append(empty ? std::string{} : std::string("name\n\xC4\x81"));
    row.Append(empty ? std::string{} : std::string(1024, 'x')); row.Append(uint32(0x12345678));
    auto source = Make(info, 0, {{1,0},{2,0},{3,0},{5,0}}, {row}, {0,0,0}); Materialized table(source, info);
    table.Locale(0); assert(table.size == 6 && !table.index[4]);
    for (uint32 id : {1u,2u,3u,5u})
    {
        assert(At<uint32>(table.index[id], 0) == id);
        assert(!std::strcmp(Local(table,id,0), empty ? "" : "name\n\xC4\x81"));
        auto second = plain ? At<char const*>(table.index[id],12) : Local(table,id,0,12);
        assert(!std::strcmp(second, empty ? "" : std::string(1024,'x').c_str()));
        assert(At<uint32>(table.index[id],20) == 0x12345678);
    }
    // A translation has an in-range ID missing in the base and a higher ID.
    MemorySource translated; translated.Append(std::string("translated")); translated.Append(std::string("plain stays original")); translated.Append(uint32(9));
    auto locale = Make(info, 2, {{1,0},{2,0},{3,0},{4,0},{6,0}}, {translated}, {0,0,0}); DB2FileLoader translation;
    assert(translation.Load(&locale,&info)); auto block = translation.AutoProduceStrings(table.index,table.size,2); assert(block); table.pools.push_back(block);
    for (uint32 id : {1u,2u,3u})
    {
        assert(!std::strcmp(Local(table,id,2),"translated"));
        assert(At<uint32>(table.index[id],20) == 0x12345678); // Localization never replaces numeric fields.
        if (plain) assert(!std::strcmp(At<char const*>(table.index[id],12),empty ? "" : std::string(1024,'x').c_str()));
        else assert(!std::strcmp(Local(table,id,2,12),"plain stays original"));
    }
    assert(!table.index[4] && !Local(table,5,2)[0]);
    assert(!translation.AutoProduceStrings(table.index,table.size,3)); // Wrong-locale file.
}
static void NumericAndInlineId()
{
    static uint8 const arrays[]{1,1,1,1,1}; DB2Meta meta(0,5,0x87654321,"ibhfl",arrays,-1);
    DB2FieldMeta fields[]{{false,FT_INT,"ID"},{false,FT_BYTE,"Byte"},{false,FT_SHORT,"Short"},{false,FT_FLOAT,"Float"},{false,FT_LONG,"Long"}};
    DB2FileLoadInfo info(fields,5,&meta); std::vector<MemorySource> rows;
    for(uint32 id : {1u,5u}){MemorySource row;row.Append(id);row.Append(uint8(0xab));row.Append(uint16(0xcdef));row.Append(float(1.25));row.Append(uint64(0x1122334455667788));rows.push_back(row);}
    auto file=Make(info,0,{{1,0},{5,1}},rows,{0,24,16,0,-32});Materialized table(file,info);table.Locale(0);
    for(uint32 id : {1u,5u}){assert(At<uint32>(table.index[id],0)==id);assert(At<uint8>(table.index[id],4)==0xab);assert(At<uint16>(table.index[id],5)==0xcdef);assert(At<float>(table.index[id],7)==1.25f);assert(At<uint64>(table.index[id],11)==0x1122334455667788);}
}
static void ArraysAndEmpty(bool plain)
{
    static uint8 const arrays[]{2,1}; DB2Meta meta(-1,2,0x10203040,"si",arrays,-1);
    DB2FieldMeta fields[]{{false,FT_INT,"ID"},{false,plain?FT_STRING_NOT_LOCALIZED:FT_STRING,"One"},
                         {false,plain?FT_STRING_NOT_LOCALIZED:FT_STRING,"Two"},{false,FT_INT,"Value"}};
    DB2FileLoadInfo info(fields,4,&meta);MemorySource row;
    row.Append(std::string("one"));row.Append(std::string("two"));row.Append(uint32(17));
    auto source=Make(info,0,{{1,0},{2,0},{5,0}},{row},{0,0});Materialized table(source,info);table.Locale(0);
    for(uint32 id : {1u,2u,5u}){
        assert(!std::strcmp(plain?At<char const*>(table.index[id],4):Local(table,id,0,4),"one"));
        assert(!std::strcmp(plain?At<char const*>(table.index[id],12):Local(table,id,0,12),"two"));
        assert(At<uint32>(table.index[id],20)==17);
    }
    auto empty=Make(info,0,{}, {},{0,0});Materialized vacant(empty,info);vacant.Locale(0);
    assert(vacant.size==2&&!vacant.index[0]&&!vacant.index[1]);
}
static void MissingBaseIds()
{
    static uint8 const arrays[]{1};DB2Meta meta(-1,1,0x11220033,"s",arrays,-1);
    DB2FieldMeta fields[]{{false,FT_INT,"ID"},{false,FT_STRING,"Name"}};DB2FileLoadInfo info(fields,2,&meta);
    MemorySource a,b;a.Append(std::string("a"));b.Append(std::string("b"));
    auto base=Make(info,0,{{1,0},{3,1}},{a,b},{0});Materialized table(base,info);table.Locale(0);
    auto extra=Make(info,2,{{1,0},{2,1},{4,1}},{a,b},{0});DB2FileLoader translated;assert(translated.Load(&extra,&info));
    auto block=translated.AutoProduceStrings(table.index,table.size,2);assert(block);table.pools.push_back(block);
    assert(!table.index[2]&&!std::strcmp(Local(table,1,2),"a")&&!Local(table,3,2)[0]);
}
static void PlainPointer()
{
    static uint8 const arrays[]{1,1,1};DB2Meta meta(-1,3,0x12345678,"ssi",arrays,-1);
    DB2FieldMeta fields[]{{false,FT_INT,"ID"},{false,FT_STRING,"Name"},{false,FT_STRING_NOT_LOCALIZED,"Plain"},{false,FT_INT,"Value"}};
    DB2FileLoadInfo info(fields,4,&meta);MemorySource row;row.Append(std::string("local"));row.Append(std::string("plain"));row.Append(uint32(1));
    auto source=Make(info,0,{{1,0}},{row},{0,0,0});Materialized table(source,info);
    assert(!std::strcmp(At<char const*>(table.index[1],12),"plain") && "sparse unlocalized field must contain the string pointer");
}
int main(int argc,char** argv)
{
    if(argc>1&&!std::strcmp(argv[1],"plain-only")){PlainPointer();std::cout<<"PASS direct unlocalized string pointer\n";return 0;}
    if(argc>1&&!std::strcmp(argv[1],"hole-only")){MissingBaseIds();std::cout<<"PASS missing base locale IDs\n";return 0;}
    bool localizedOnly = argc > 1 && !std::strcmp(argv[1], "localized-only");
    Strings(false,false); Strings(false,true);
    if(!localizedOnly){Strings(true,false);Strings(true,true);NumericAndInlineId();ArraysAndEmpty(false);ArraysAndEmpty(true);MissingBaseIds();PlainPointer();}
    std::cout << "PASS generated WDC1 alias IDs, holes, empty/UTF8/long strings, locale reload/missing base IDs/wrong locale, mixed localized/plain fields, native numeric widths and inline IDs\n";
    return 0;
}
