#include "BPT.hpp"
#include <cstring>
#include <iostream>
struct Key65
{
    char data[65];

    Key65()
    {
        data[0] = '\0';
    }

    explicit Key65(const std::string &s)
    {
        std::strncpy(data, s.c_str(), 64);
        data[64] = '\0';
    }

    inline bool operator<(const Key65 &other) const
    {
        const unsigned long long *a = reinterpret_cast<const unsigned long long *>(data);
        const unsigned long long *b = reinterpret_cast<const unsigned long long *>(other.data);

        // 前 8 字节
        if (a[0] != b[0]) return a[0] < b[0];
        if (a[1] != b[1]) return a[1] < b[1];
        if (a[2] != b[2]) return a[2] < b[2];
        if (a[3] != b[3]) return a[3] < b[3];
        if (a[4] != b[4]) return a[4] < b[4];
        if (a[5] != b[5]) return a[5] < b[5];
        if (a[6] != b[6]) return a[6] < b[6];
        if (a[7] != b[7]) return a[7] < b[7];

        return false;
    }

    inline bool operator==(const Key65 &other) const
    {
        const unsigned long long *a = reinterpret_cast<const unsigned long long *>(data);
        const unsigned long long *b = reinterpret_cast<const unsigned long long *>(other.data);

        for (int i = 0; i < 8; ++i)
        {
            if (a[i] != b[i])
                return false;
        }

        return true;
    }

    inline bool operator!=(const Key65 &other) const
    {
        return !(*this == other);
    }

    inline bool operator<=(const Key65 &other) const
    {
        return !(*this > other);
    }

    inline bool operator>(const Key65 &other) const
    {
        const unsigned long long *a = reinterpret_cast<const unsigned long long *>(data);
        const unsigned long long *b = reinterpret_cast<const unsigned long long *>(other.data);

        if (a[0] != b[0]) return a[0] > b[0];
        if (a[1] != b[1]) return a[1] > b[1];
        if (a[2] != b[2]) return a[2] > b[2];
        if (a[3] != b[3]) return a[3] > b[3];
        if (a[4] != b[4]) return a[4] > b[4];
        if (a[5] != b[5]) return a[5] > b[5];
        if (a[6] != b[6]) return a[6] > b[6];
        if (a[7] != b[7]) return a[7] > b[7];

        return false;
    }
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