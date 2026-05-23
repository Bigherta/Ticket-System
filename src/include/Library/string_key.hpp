#ifndef STRING_KEY_HPP
#define STRING_KEY_HPP
#include <cstring>
namespace sjtu {
    template<size_t N>
    struct StringKey {
        char str[N];
        StringKey() { std::memset(str, 0, N); }
        StringKey(const char* s) { std::memset(str, 0, N); std::strncpy(str, s, N - 1); }
        StringKey(const StringKey& other) { std::memcpy(str, other.str, N); }
        StringKey& operator=(const StringKey& other) { if (this != &other) std::memcpy(str, other.str, N); return *this; }
        bool operator<(const StringKey& other) const { return std::strcmp(str, other.str) < 0; }
        bool operator==(const StringKey& other) const { return std::strcmp(str, other.str) == 0; }
        bool operator!=(const StringKey& other) const { return !(*this == other); }
    };
}
#endif
