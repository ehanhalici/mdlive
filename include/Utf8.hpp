#include <cstdint>
#include <string>

uint32_t utf8_decode_next(const char*& p, const char* end) ;
int mk_wcwidth(uint32_t ucs);
size_t utf8_width(const std::string& str);
