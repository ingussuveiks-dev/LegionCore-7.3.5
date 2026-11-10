#include <cstdint>
#include <stdexcept>
#include <iostream>
using uint32=uint32_t;
static void Require(bool ok,char const*why){if(!ok)throw std::runtime_error(why);}
#include "Selected.inc"
int main(int argc,char**argv){try{
#include "Check.inc"
std::cout<<"PASS reset first-use/retry contract: "<<(argc>1?argv[1]:"")<<";old reads replaced by explicit diagnostic sentinel;no client encounter claim\n";return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
