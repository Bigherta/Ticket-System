#ifndef SJTU_VECTOR_HPP
#define SJTU_VECTOR_HPP

#include "exceptions.hpp"

namespace sjtu
{
    /**
     * a data container like std::vector
     * store data in a successive memory and support random access.
     */
    template<typename T>
    class vector
    {
    public:
        /**
         * TODO
         * a type for actions of the elements of a vector, and you should write
         *   a class named const_iterator with same interfaces.
         */
        /**
         * you can see RandomAccessIterator at CppReference for help.
         */
        T *data;
        size_t capacity;
        size_t length;
        class const_iterator;
        class iterator
        {
            // The following code is written for the C++ type_traits library.
            // Type traits is a C++ feature for describing certain properties of a
            // type. For instance, for an iterator, iterator::value_type is the type
            // that the iterator points to. STL algorithms and containers may use
            // these type_traits (e.g. the following typedef) to work properly. In
            // particular, without the following code,
            // @code{std::sort(iter, iter1);} would not compile.
            // See these websites for more information:
            // https://en.cppreference.com/w/cpp/header/type_traits
            // About value_type:
            // https://blog.csdn.net/u014299153/article/details/72419713 About
            // iterator_category: https://en.cppreference.com/w/cpp/iterator
        public:
            using value_type = T;
            using pointer = T *;
            using reference = T &;
            using iterator_category = std::output_iterator_tag;

        private:
            /**
             * TODO add data members
             *   just add whatever you want.
             */
            T *ptr = nullptr;
            vector *vec = nullptr;

        public:
            iterator(T *ptr_ = nullptr, vector *vec_ = nullptr) noexcept : ptr(ptr_), vec(vec_) {}
            /**
             * return a new iterator which pointer n-next elements
             * as well as operator-
             */
            iterator operator+(const int &n) const noexcept
            {
                // TODO
                return iterator(ptr + n, vec);
            }
            iterator operator-(const int &n) const noexcept
            {
                // TODO
                return iterator(ptr - n, vec);
            }
            // return the distance between two iterators,
            // if these two iterators point to different vectors, throw
            // invaild_iterator.
            int operator-(const iterator &rhs) const
            {
                // TODO
                if (vec != rhs.vec)
                    throw invalid_iterator();
                return ptr - rhs.ptr;
            }
            iterator &operator+=(const int &n) noexcept
            {
                // TODO
                ptr += n;
                return *this;
            }
            iterator &operator-=(const int &n) noexcept
            {
                // TODO
                ptr -= n;
                return *this;
            }
            /**
             * TODO iter++
             */
            iterator operator++(int) noexcept
            {
                iterator temp = *this;
                ++ptr;
                return temp;
            }
            /**
             * TODO ++iter
             */
            iterator &operator++() noexcept
            {
                ++ptr;
                return *this;
            }
            /**
             * TODO iter--
             */
            iterator operator--(int) noexcept
            {
                iterator temp = *this;
                --ptr;
                return temp;
            }
            /**
             * TODO --iter
             */
            iterator &operator--() noexcept
            {
                --ptr;
                return *this;
            }
            /**
             * TODO *it
             */
            T &operator*() const noexcept { return *ptr; }
            /**
             * an operator to check whether two iterators are same (pointing to the
             * same memory address).
             */
            bool operator==(const iterator &rhs) const noexcept
            {
                if (vec == rhs.vec && ptr == rhs.ptr)
                    return true;
                return false;
            }
            bool operator==(const const_iterator &rhs) const noexcept
            {
                if (vec == rhs.vec && ptr == rhs.ptr)
                    return true;
                return false;
            }
            /**
             * some other operator for iterator.
             */
            bool operator!=(const iterator &rhs) const noexcept { return !(*this == rhs); }
            bool operator!=(const const_iterator &rhs) const noexcept { return !(*this == rhs); }
        };
        /**
         * TODO
         * has same function as iterator, just for a const object.
         */
        class const_iterator
        {
        public:
            using value_type = T;
            using pointer = T *;
            using reference = T &;
            using iterator_category = std::output_iterator_tag;
            const_iterator(const T *ptr_ = nullptr, const vector *vec_ = nullptr) : ptr(ptr_), vec(vec_) {}
            const_iterator(const iterator &other) : ptr(other.ptr), vec(other.vec) {}
            /**
             * return a new iterator which pointer n-next elements
             * as well as operator-
             */
            const_iterator operator+(const int &n) const noexcept
            {
                // TODO
                return const_iterator(ptr + n, vec);
            }
            const_iterator operator-(const int &n) const noexcept
            {
                // TODO
                return const_iterator(ptr - n, vec);
            }
            // return the distance between two iterators,
            // if these two iterators point to different vectors, throw
            // invaild_iterator.
            int operator-(const const_iterator &rhs) const
            {
                // TODO
                if (vec != rhs.vec)
                    throw invalid_iterator();
                return ptr - rhs.ptr;
            }
            const_iterator &operator+=(const int &n) noexcept
            {
                // TODO
                ptr += n;
                return *this;
            }
            const_iterator &operator-=(const int &n) noexcept
            {
                // TODO
                ptr -= n;
                return *this;
            }
            /**
             * TODO iter++
             */
            const_iterator operator++(int) noexcept
            {
                const_iterator temp = *this;
                ++ptr;
                return temp;
            }
            /**
             * TODO ++iter
             */
            const_iterator &operator++() noexcept
            {
                ++ptr;
                return *this;
            }
            /**
             * TODO iter--
             */
            const_iterator operator--(int) noexcept
            {
                const_iterator temp = *this;
                --ptr;
                return temp;
            }
            /**
             * TODO --iter
             */
            const_iterator &operator--() noexcept
            {
                --ptr;
                return *this;
            }
            /**
             * TODO *it
             */
            const T &operator*() const noexcept { return *ptr; }
            /**
             * an operator to check whether two iterators are same (pointing to the
             * same memory address).
             */
            bool operator==(const iterator &rhs) const noexcept
            {
                if (vec == rhs.vec && ptr == rhs.ptr)
                    return true;
                return false;
            }
            bool operator==(const const_iterator &rhs) const noexcept
            {
                if (vec == rhs.vec && ptr == rhs.ptr)
                    return true;
                return false;
            }
            /**
             * some other operator for iterator.
             */
            bool operator!=(const iterator &rhs) const noexcept { return !(*this == rhs); }
            bool operator!=(const const_iterator &rhs) const noexcept { return !(*this == rhs); }

        private:
            /*TODO*/
            const T *ptr = nullptr;
            const vector *vec = nullptr;
        };
        /**
         * TODO Constructs
         * At least two: default constructor, copy constructor
         */
        vector() noexcept : capacity(256), length(0) { data = static_cast<T *>(::operator new(capacity * sizeof(T))); }
        vector(const vector &other) noexcept : length(other.length), capacity(other.capacity)
        {
            data = static_cast<T *>(::operator new(capacity * sizeof(T)));
            for (size_t i = 0; i < length; ++i)
            {
                new (data + i) T(other.data[i]);
            }
        }
        vector(vector &&other) noexcept : capacity(other.capacity), length(other.length)
        {
            data = other.data;
            other.capacity = 0;
            other.length = 0;
            other.data = nullptr;
        }
        /**
         * TODO Destructor
         */
        ~vector() noexcept
        {
            for (size_t i = 0; i < length; ++i)
                data[i].~T();
            ::operator delete(data);
            capacity = 0;
            length = 0;
        }
        /**
         * TODO Assignment operator
         */
        vector &operator=(const vector &other)
        {
            if (this == &other)
                return *this;
            for (size_t i = 0; i < length; ++i)
                data[i].~T();
            ::operator delete(data);
            length = other.length;
            capacity = other.capacity;
            data = static_cast<T *>(::operator new(capacity * sizeof(T)));

            for (size_t i = 0; i < length; ++i)
            {
                new (data + i) T(other.data[i]);
            }
            return *this;
        }
        vector &operator=(vector &&other) noexcept
        {
            if (this != &other)
            {
                std::swap(data, other.data);
                std::swap(length, other.length);
                std::swap(capacity, other.capacity);
            }
            return *this;
        }

        /**
         * assigns specified element with bounds checking
         * throw index_out_of_bound if pos is not in [0, size)
         */
        T &at(const size_t &pos)
        {
            if (pos >= length)
                throw index_out_of_bound();
            return data[pos];
        }
        const T &at(const size_t &pos) const
        {
            if (pos >= length)
                throw index_out_of_bound();
            return data[pos];
        }
        /**
         * assigns specified element with bounds checking
         * throw index_out_of_bound if pos is not in [0, size)
         * !!! Pay attentions
         *   In STL this operator does not check the boundary but I want you to do.
         */
        T &operator[](const size_t &pos)
        {
            if (pos >= length)
                throw index_out_of_bound();
            return data[pos];
        }
        const T &operator[](const size_t &pos) const
        {
            if (pos >= length)
                throw index_out_of_bound();
            return data[pos];
        }
        /**
         * access the first element.
         * throw container_is_empty if size == 0
         */
        const T &front() const
        {
            if (length == 0)
                throw container_is_empty();
            return data[0];
        }
        /**
         * access the last element.
         * throw container_is_empty if size == 0
         */
        const T &back() const
        {
            if (length == 0)
                throw container_is_empty();
            return data[length - 1];
        }
        /**
         * returns an iterator to the beginning.
         */
        iterator begin() noexcept { return iterator(data, this); }
        const_iterator begin() const noexcept { return const_iterator(data, this); }
        const_iterator cbegin() const noexcept { return const_iterator(data, this); }
        /**
         * returns an iterator to the end.
         */
        iterator end() noexcept { return iterator(data + length, this); }
        const_iterator end() const noexcept { return const_iterator(data + length, this); }
        const_iterator cend() const noexcept { return const_iterator(data + length, this); }
        /**
         * checks whether the container is empty
         */
        bool empty() const noexcept { return length == 0; }
        /**
         * returns the number of elements
         */
        size_t size() const noexcept { return length; }
        /**
         * clears the contents
         */
        void clear() noexcept
        {
            for (size_t i = 0; i < length; ++i)
                data[i].~T();
            length = 0;
        }
        /**
         * inserts value before pos
         * returns an iterator pointing to the inserted value.
         */
        iterator insert(iterator pos, const T &value) noexcept
        {
            int index = pos - begin();

            if (length + 1 > capacity)
            {
                capacity *= 2;
                T *newdata = static_cast<T *>(::operator new(capacity * sizeof(T)));
                for (size_t i = 0; i < length; ++i)
                {
                    new (newdata + i) T(std::move(data[i]));
                }
                for (size_t i = 0; i < length; ++i)
                    data[i].~T();
                ::operator delete(data);
                data = newdata;
            }

            for (size_t i = length; i > index; --i)
            {
                new (data + i) T(std::move(data[i - 1]));
            }

            new (data + index) T(std::move(value));


            ++length;

            return iterator(data + index, this);
        }
        /**
         * inserts value at index ind.
         * after inserting, this->at(ind) == value
         * returns an iterator pointing to the inserted value.
         * throw index_out_of_bound if ind > size (in this situation ind can be size
         * because after inserting the size will increase 1.)
         */
        iterator insert(const size_t &ind, const T &value)
        {
            if (ind > length)
                throw index_out_of_bound();
            return insert(iterator(data + ind, this), value);
        }
        /**
         * removes the element at pos.
         * return an iterator pointing to the following element.
         * If the iterator pos refers the last element, the end() iterator is
         * returned.
         */
        iterator erase(iterator pos) noexcept
        {
            int index = pos - begin();
            if (index < length - 1)
            {
                for (size_t i = index; i < length - 1; ++i)
                {
                    new (data + i) T(std::move(data[i + 1]));
                }
            }
            data[length - 1].~T();
            --length;
            return (index >= length) ? end() : iterator(data + index, this);
        }
        /**
         * removes the element with index ind.
         * return an iterator pointing to the following element.
         * throw index_out_of_bound if ind >= size
         */
        iterator erase(const size_t &ind)
        {
            if (ind >= length)
                throw index_out_of_bound();
            if (ind < length - 1)
            {
                for (size_t i = ind; i < length - 1; ++i)
                {
                    new (data + i) T(std::move(data[i + 1]));
                }
            }
            data[length - 1].~T();
            --length;
            return (ind >= length) ? end() : iterator(data + ind, this);
        }
        /**
         * adds an element to the end.
         */
        void push_back(const T &value) noexcept { insert(end(), value); }
        /**
         * adds an rvalue element to the end.
         */
        void push_back(T &&value) noexcept { insert(end(), std::move(value)); }
        /**
         * remove the last element from the end.
         * throw container_is_empty if size() == 0
         */
        void pop_back()
        {
            if (length == 0)
                throw container_is_empty();
            erase(end() - 1);
        }
    };

} // namespace sjtu

#endif
