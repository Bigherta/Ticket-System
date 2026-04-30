#ifndef SJTU_UNORDERED_MAP_HPP
#define SJTU_UNORDERED_MAP_HPP

#include <string>
#include <cstring>
#include "vector.hpp"
#include "utility.hpp"

namespace sjtu
{

    namespace detail
    {
        static constexpr unsigned long long FNV_OFFSET = 1469598103934665603ULL;
        static constexpr unsigned long long FNV_PRIME = 1099511628211ULL;

        static inline size_t HashBytes(const unsigned char *data, size_t n)
        {
            unsigned long long h = FNV_OFFSET;
            size_t i = 0;
            // process 8-byte blocks (use memcpy for safe unaligned access)
            for (; i + 8 <= n; i += 8)
            {
                unsigned long long chunk;
                memcpy(&chunk, data + i, 8);
                h ^= chunk;
                h *= FNV_PRIME;
            }
            // tail
            for (; i < n; ++i)
            {
                h ^= data[i];
                h *= FNV_PRIME;
            }
            return static_cast<size_t>(h);
        }

        static inline size_t hash_string(const std::string &s)
        {
            const size_t n = s.size();
            if (n == 0) return static_cast<size_t>(FNV_OFFSET);
            const unsigned char *data = reinterpret_cast<const unsigned char *>(s.data());

            // fast path: process 8-byte chunks using unsigned long long pointer for throughput
            unsigned long long h = FNV_OFFSET;
            size_t blocks = n / 8;
            const unsigned long long *p = reinterpret_cast<const unsigned long long *>(data);
            for (size_t i = 0; i < blocks; ++i)
            {
                h ^= p[i];
                h *= FNV_PRIME;
            }
            // tail bytes
            size_t offset = blocks * 8;
            for (size_t i = offset; i < n; ++i)
            {
                h ^= data[i];
                h *= FNV_PRIME;
            }
            return static_cast<size_t>(h);
        }

        // generic byte-hash fallback for arbitrary types
        template<typename T>
        struct HashKey
        {
            static inline size_t eval(const T &key)
            {
                return HashBytes(reinterpret_cast<const unsigned char *>(&key), sizeof(T));
            }
        };

        // int specialization: Knuth multiplicative hash (fast)
        template<>
        struct HashKey<int>
        {
            static inline size_t eval(int x)
            {
                return static_cast<size_t>(static_cast<unsigned long long>(x) * 11995408973635179863ULL);
            }
        };

        template<>
        struct HashKey<std::string>
        {
            static inline size_t eval(const std::string &s) { return hash_string(s); }
        };
    } // namespace detail

    template<class Key, class Value>
    class unordered_map
    {
    public:
        using value_type = sjtu::pair<Key, Value>;
        using size_type = size_t;

    private:
        struct node
        {
            value_type val;
            size_type hash_val;
            node *next;
            node(const value_type &v, size_type h, node *n = nullptr) : val(v), hash_val(h), next(n) {}
        };

        sjtu::vector<node *> buckets;
        size_type bucket_count_ = 0;
        size_type size_ = 0;
        double max_load_factor_ = 0.75;

        static size_type next_pow2(size_type n)
        {
            size_type p = 1;
            while (p < n)
                p <<= 1;
            return p;
        }

        void rehash(size_type new_count)
        {
            new_count = next_pow2(new_count);
            sjtu::vector<node *> new_buckets;
            for (size_type i = 0; i < new_count; ++i)
                new_buckets.push_back(nullptr);

            for (size_type i = 0; i < bucket_count_; ++i)
            {
                node *cur = buckets[i];
                while (cur)
                {
                    node *next = cur->next;
                    size_type idx = cur->hash_val & (new_count - 1);
                    cur->next = new_buckets[idx];
                    new_buckets[idx] = cur;
                    cur = next;
                }
            }

            buckets = new_buckets;
            bucket_count_ = new_count;
        }

        void check_rehash()
        {
            if (bucket_count_ == 0)
                rehash(1);
            else if (size_ > static_cast<size_type>(bucket_count_ * max_load_factor_))
                rehash(bucket_count_ * 2);
        }

    public:
        unordered_map(size_type bucket_cnt = 1024, double max_load_factor = 0.75)
            : bucket_count_(next_pow2(bucket_cnt)), size_(0), max_load_factor_(max_load_factor)
        {
            for (size_type i = 0; i < bucket_count_; ++i)
                buckets.push_back(nullptr);
        }

        // 禁用拷贝/移动以避免裸指针重复释放
        unordered_map(const unordered_map &) = delete;
        unordered_map(unordered_map &&) = delete;
        unordered_map &operator=(const unordered_map &) = delete;
        unordered_map &operator=(unordered_map &&) = delete;

        ~unordered_map()
        {
            clear();
        }

        void clear()
        {
            for (size_type i = 0; i < bucket_count_; ++i)
            {
                node *cur = buckets[i];
                while (cur)
                {
                    node *next = cur->next;
                    delete cur;
                    cur = next;
                }
                buckets[i] = nullptr;
            }
            size_ = 0;
        }

        class iterator
        {
            friend class unordered_map;

        private:
            unordered_map *mp = nullptr;
            size_type idx = 0;
            node *cur = nullptr;

        public:
            iterator() = default;
            iterator(unordered_map *m, size_type i, node *c) : mp(m), idx(i), cur(c) {}

            value_type &operator*() const { return cur->val; }
            value_type *operator->() const { return &(cur->val); }

            bool operator==(const iterator &rhs) const { return mp == rhs.mp && cur == rhs.cur; }

            bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
        };

        iterator begin()
        {
            if (size_ == 0) return end();
            for (size_type i = 0; i < bucket_count_; ++i)
                if (buckets[i])
                    return iterator(this, i, buckets[i]);
            return end();
        }

        iterator end() { return iterator(this, bucket_count_, nullptr); }

        iterator find(const Key &key)
        {
            size_type hval = static_cast<size_type>(detail::HashKey<Key>::eval(key));
            size_type idx = hval & (bucket_count_ - 1);
            node *cur = buckets[idx];

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
            size_type hval = static_cast<size_type>(detail::HashKey<Key>::eval(value.first));
            size_type idx = hval & (bucket_count_ - 1);

            node *cur = buckets[idx];
            while (cur)
            {
                if (cur->hash_val == hval && cur->val.first == value.first)
                {
                    cur->val.second = value.second;
                    return {iterator(this, idx, cur), false};
                }
                cur = cur->next;
            }

            node *n = new node(value, hval, buckets[idx]);
            buckets[idx] = n;
            ++size_;
            check_rehash();

            return {iterator(this, idx, n), true};
        }

        bool erase(const Key &key)
        {
            size_type hval = static_cast<size_type>(detail::HashKey<Key>::eval(key));
            size_type idx = hval & (bucket_count_ - 1);

            node *cur = buckets[idx];
            node *prev = nullptr;

            while (cur)
            {
                if (cur->hash_val == hval && cur->val.first == key)
                {
                    if (prev)
                        prev->next = cur->next;
                    else
                        buckets[idx] = cur->next;

                    delete cur;
                    --size_;
                    return true;
                }
                prev = cur;
                cur = cur->next;
            }
            return false;
        }

        bool erase(const iterator &pos)
        {
            if (pos.mp != this || pos.cur == nullptr)
                return false;

            size_type idx = pos.idx;
            node *cur = buckets[idx];
            node *prev = nullptr;

            while (cur)
            {
                if (cur == pos.cur)
                {
                    if (prev)
                        prev->next = cur->next;
                    else
                        buckets[idx] = cur->next;

                    delete cur;
                    --size_;
                    return true;
                }
                prev = cur;
                cur = cur->next;
            }
            return false;
        }

        Value &operator[](const Key &key)
        {
            size_type hval = static_cast<size_type>(detail::HashKey<Key>::eval(key));
            size_type idx = hval & (bucket_count_ - 1);
            node *cur = buckets[idx];
            while (cur)
            {
                if (cur->hash_val == hval && cur->val.first == key)
                    return cur->val.second;
                cur = cur->next;
            }

            value_type val(key, Value());
            node *n = new node(val, hval, buckets[idx]);
            buckets[idx] = n;
            ++size_;
            check_rehash();
            return n->val.second;
        }

        size_type size() const { return size_; }
        size_type bucket_count() const { return bucket_count_; }
        double load_factor() const { return bucket_count_ ? static_cast<double>(size_) / bucket_count_ : 0.0; }
        double max_load_factor() const { return max_load_factor_; }
        void reserve(size_type n) { rehash(next_pow2(static_cast<size_type>(n / max_load_factor_) + 1)); }
    };

} // namespace sjtu

#endif // SJTU_UNORDERED_MAP_HPP
