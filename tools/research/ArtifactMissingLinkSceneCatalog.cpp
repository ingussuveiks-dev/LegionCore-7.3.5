#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>

// Read client scene text as evidence only. Numeric matches can belong to other
// namespaces; neither a hit nor an absence establishes a server SmartAI action.
int main(int argc, char** argv)
{
    if (argc != 2)
        return 1;

    DB2Storage<SceneScriptTextEntry> texts("SceneScriptText.db2", SceneScriptTextLoadInfo::Instance());
    if (!texts.Load(argv[1], 0))
        return 2;

    std::vector<std::string> const terms{
        "wardstone", "talonclaw", "tideskorn", "dakarr", "apata", "wuho",
        "101325", "101360", "199728", "199770", "199823", "187027",
        "101430", "103151", "200165", "201004", "48121", "48160", "48172"
    };
    uint32 records = 0, matches = 0;
    for (auto text : texts)
    {
        ++records;
        std::string name = text->Name->Str[0], script = text->Script->Str[0];
        std::string lower = name + '\n' + script;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return char(std::tolower(c)); });
        std::vector<std::string> found;
        for (auto const& term : terms)
            if (lower.find(term) != std::string::npos)
                found.push_back(term);
        if (found.empty())
            continue;

        ++matches;
        std::cout << "MATCH\t" << text->ID << '\t' << name;
        for (auto const& term : found)
            std::cout << '\t' << term;
        std::cout << "\nSCRIPT_BEGIN\t" << text->ID << '\t' << script.size() << '\n'
                  << script << "\nSCRIPT_END\t" << text->ID << '\n';
    }
    std::cout << "COMPLETE\t" << records << '\t' << matches << '\n';
    return records == 5969 ? 0 : 3;
}
