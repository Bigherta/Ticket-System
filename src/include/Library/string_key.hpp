#ifndef STRING_KEY_HPP
#define STRING_KEY_HPP
#include <cstring>
#include <string>
namespace sjtu {
    template<size_t N>
    struct StringKey {
        char str[N];
        StringKey() { std::memset(str, 0, N); }
        StringKey(const char* s) {
            std::memset(str, 0, N);
            if (s) std::strncpy(str, s, N - 1);
        }
        StringKey(const std::string &s) {
            std::memset(str, 0, N);
            size_t len = s.size();
            if (len > N - 1) len = N - 1;
            if (len) std::memcpy(str, s.data(), len);
        }
        StringKey(const StringKey& other) { std::memcpy(str, other.str, N); }
        StringKey& operator=(const StringKey& other) { if (this != &other) std::memcpy(str, other.str, N); return *this; }
        static int cmp(const StringKey& a, const StringKey& b) { return std::memcmp(a.str, b.str, N - 1); }
        bool operator<(const StringKey& other) const { return cmp(*this, other) < 0; }
        bool operator==(const StringKey& other) const { return cmp(*this, other) == 0; }
        bool operator!=(const StringKey& other) const { return !(*this == other); }
        bool operator<=(const StringKey& other) const { return cmp(*this, other) <= 0; }
    };
}
#endif
