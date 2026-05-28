#include "BPT.hpp"
#include <cstring>
#include <iostream>

struct Key65
{
    char data[65];

    Key65() { std::memset(data, 0, sizeof(data)); }

    explicit Key65(const std::string &s)
    {
        std::memset(data, 0, sizeof(data));
        size_t len = s.size();
        if (len > 64)
            len = 64;
        std::memcpy(data, s.data(), len);
    }

    static int cmp(const Key65 &a, const Key65 &b)
    {
        return std::memcmp(a.data, b.data, 64);
    }

    bool operator<(const Key65 &other) const { return cmp(*this, other) < 0; }
    bool operator==(const Key65 &other) const { return cmp(*this, other) == 0; }
    bool operator!=(const Key65 &other) const { return !(*this == other); }
    bool operator<=(const Key65 &other) const { return cmp(*this, other) <= 0; }
};

int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);
    int n;
    std::cin >> n;
    BPT<Key65> bpt;
    for (int i = 0; i < n; ++i)
    {
        std::string op;
        std::cin >> op;
        if (op == "insert")
        {
            std::string key;
            int value;
            std::cin >> key >> value;
            bpt.insert(Key65(key), value);
        }
        else if (op == "delete")
        {
            std::string key;
            int value;
            std::cin >> key >> value;
            bpt.remove(Key65(key), value);
        }
        else if (op == "find")
        {
            std::string key;
            std::cin >> key;
            bpt.search(Key65(key));
        }
    }
    return 0;
}