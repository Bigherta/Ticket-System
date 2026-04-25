#ifndef SJTU_UNORDERED_MAP_HPP
#define SJTU_UNORDERED_MAP_HPP
#include <string>
#include "utility.hpp"

namespace sjtu
{

    static inline unsigned long long hash(const std::string &str)
    {
        unsigned long long h = 1469598103934665603ULL;
        for (size_t i = 0; i < str.size(); ++i)
        {
            h ^= (unsigned long long) (unsigned char) str[i];
            h *= 1099511628211ULL;
            h ^= (h << 13);
            h ^= (h >> 7);
            h ^= (h << 17);
        }
        return h;
    }

    template<class T>
    class unordered_map
    {
    public:
        using value_type = sjtu::pair<std::string, T>;

    private:
        struct node
        {
            value_type val;
            size_t hash_val;
            node *next = nullptr;

            node(const value_type &v, size_t h, node *n = nullptr) : val(v), hash_val(h), next(n) {}
        };

        static constexpr double load_factor = 0.75;

        node **table = nullptr;
        size_t capacity = 1024;
        size_t size_ = 0;

        void rehash()
        {
            size_t old_cap = capacity;
            capacity *= 2;

            node **new_table = new node *[capacity]();
            for (size_t i = 0; i < old_cap; ++i)
            {
                node *cur = table[i];
                while (cur)
                {
                    node *next = cur->next;

                    size_t idx = cur->hash_val & (capacity - 1);
                    cur->next = new_table[idx];
                    new_table[idx] = cur;

                    cur = next;
                }
            }

            delete[] table;
            table = new_table;
        }

    public:
        unordered_map() { table = new node *[capacity](); }
        unordered_map(std::initializer_list<value_type> init) : unordered_map()
        {
            for (const auto &v: init)
                insert(v);
        }

        // 禁用拷贝/移动以避免裸指针重复释放
        unordered_map(const unordered_map &) = delete;
        unordered_map(unordered_map &&) = delete;
        unordered_map &operator=(const unordered_map &) = delete;
        unordered_map &operator=(unordered_map &&) = delete;

        ~unordered_map()
        {
            clear();
            delete[] table;
        }

        void clear()
        {
            if (!table) return;
            for (size_t i = 0; i < capacity; ++i)
            {
                node *cur = table[i];
                while (cur)
                {
                    node *next = cur->next;
                    delete cur;
                    cur = next;
                }
                table[i] = nullptr;
            }
            size_ = 0;
        }

        class iterator
        {
            friend class unordered_map;

        private:
            unordered_map *mp = nullptr;
            size_t idx = 0;
            node *cur = nullptr;

        public:
            iterator() = default;
            iterator(unordered_map *m, size_t i, node *c) : mp(m), idx(i), cur(c) {}

            value_type &operator*() const { return cur->val; }
            value_type *operator->() const { return &(cur->val); }

            bool operator==(const iterator &rhs) const { return mp == rhs.mp && cur == rhs.cur; }

            bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
        };

        iterator begin()
        {
            if (!table) return end();
            for (size_t i = 0; i < capacity; ++i)
                if (table[i])
                    return iterator(this, i, table[i]);
            return end();
        }

        iterator end() { return iterator(this, capacity, nullptr); }

        iterator find(const std::string &key)
        {
            size_t hval = hash(key);
            size_t idx = hval & (capacity - 1);
            node *cur = table[idx];

            while (cur)
            {
                if (cur->hash_val == hval && cur->val.first == key)
                    return iterator(this, idx, cur);
                cur = cur->next;
            }
            return end();
        }

        pair<iterator, bool> insert(const value_type &value)
        {
            if ((size_ + 1) * 4 > capacity * 3)
                rehash();

            size_t hval = hash(value.first);
            size_t idx = hval & (capacity - 1);

            node *cur = table[idx];
            while (cur)
            {
                if (cur->hash_val == hval && cur->val.first == value.first)
                {
                    cur->val.second = value.second;
                    return {iterator(this, idx, cur), false};
                }
                cur = cur->next;
            }

            node *n = new node(value, hval, table[idx]);
            table[idx] = n;
            ++size_;

            return {iterator(this, idx, n), true};
        }

        bool remove(const std::string &key)
        {
            size_t hval = hash(key);
            size_t idx = hval & (capacity - 1);

            node *cur = table[idx];
            node *prev = nullptr;

            while (cur)
            {
                if (cur->hash_val == hval && cur->val.first == key)
                {
                    if (prev)
                        prev->next = cur->next;
                    else
                        table[idx] = cur->next;

                    delete cur;
                    --size_;
                    return true;
                }
                prev = cur;
                cur = cur->next;
            }
            return false;
        }

        size_t size() const { return size_; }
    };

} // namespace sjtu

#endif
