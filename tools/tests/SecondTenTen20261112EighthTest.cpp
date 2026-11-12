#include <string>
#include "SecondTenTen20261112EighthClockFixture.h"
#include "SecondTenTen20261112EighthAuraFixture.h"
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("case");int test=std::stoi(argv[1]);if(test<8)EighthClock::RunClockCase(test);else EighthAura::RunAuraCase(test);std::cout<<"PASS complete production combat callbacks and AuraScript classes; controlled world endpoints, actual native EventMap and native aura dispatch remain separate scopes\n";return 0;}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
