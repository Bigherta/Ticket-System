#ifndef SJTU_PRIORITY_QUEUE_HPP
#define SJTU_PRIORITY_QUEUE_HPP

#include <utility>
#include "exceptions.hpp"

namespace sjtu
{
    /**
     * @brief a container like std::priority_queue which is a heap internal.
     * **Exception Safety**: The `Compare` operation might throw exceptions for certain data.
     * In such cases, any ongoing operation should be terminated, and the priority queue should be restored to its
     * original state before the operation began.
     */
    template<typename T, class Compare = std::less<T>>
    class priority_queue
    {
    private:
        T *data;
        size_t length;
        size_t maxsize = 100;
        void destroy_range(size_t left, size_t right)
        {
            for (size_t i = left; i <= right; ++i)
            {
                data[i].~T();
            }
        }

        void double_space()
        {
            size_t new_maxsize = maxsize * 2;
            T *new_data = static_cast<T *>(::operator new(new_maxsize * sizeof(T)));
            size_t built = 0;
            for (size_t i = 1; i <= length; ++i)
            {
                try
                {
                    new (new_data + i) T(data[i]);
                    ++built;
                }
                catch (...)
                {
                    for (size_t j = 1; j <= built; ++j)
                    {
                        new_data[j].~T();
                    }
                    ::operator delete(new_data);
                    throw;
                }
            }
            if (length > 0)
            {
                destroy_range(1, length);
            }
            ::operator delete(data);
            data = new_data;
            maxsize = new_maxsize;
        }

    public:
        /**
         * @brief default constructor
         */
        priority_queue()
        {
            data = static_cast<T *>(::operator new(maxsize * sizeof(T)));
            length = 0;
        }

        /**
         * @brief copy constructor
         * @param other the priority_queue to be copied
         */
        priority_queue(const priority_queue &other)
        {
            length = other.length;
            maxsize = other.maxsize;
            data = static_cast<T *>(::operator new(maxsize * sizeof(T)));
            size_t built = 0;
            for (size_t i = 1; i <= length; ++i)
            {
                try
                {
                    new (data + i) T(other.data[i]);
                    ++built;
                }
                catch (...)
                {
                    for (size_t j = 1; j <= built; ++j)
                    {
                        data[j].~T();
                    }
                    ::operator delete(data);
                    throw;
                }
            }
        }

        /**
         * @brief deconstructor
         */
        ~priority_queue()
        {
            if (length > 0)
            {
                destroy_range(1, length);
            }
            ::operator delete(data);
            maxsize = 0;
            length = 0;
        }

        /**
         * @brief Assignment operator
         * @param other the priority_queue to be assigned from
         * @return a reference to this priority_queue after assignment
         */
        priority_queue &operator=(const priority_queue &other)
        {
            if (this == &other)
            {
                return *this;
            }
            priority_queue tmp(other);
            std::swap(data, tmp.data);
            std::swap(length, tmp.length);
            std::swap(maxsize, tmp.maxsize);
            return *this;
        }

        /**
         * @brief get the top element of the priority queue.
         * @return a reference of the top element.
         * @throws container_is_empty if empty() returns true
         */
        const T &top() const
        {
            if (this->empty())
            {
                throw container_is_empty();
            }
            return data[1];
        }

        /**
         * @brief push new element to the priority queue.
         * @param e the element to be pushed
         */
        void push(const T &e)
        {
            T value = e;
            if (length + 1 == maxsize)
            {
                double_space();
            }

            size_t hole = length + 1;
            try
            {
                for (; hole > 1 && Compare()(data[hole / 2], value); hole /= 2)
                {
                }
            }
            catch (...)
            {
                throw;
            }

            size_t pos = length + 1;
            new (data + pos) T(value);
            for (; pos > hole; pos /= 2)
            {
                data[pos] = data[pos / 2];
            }
            data[hole] = value;
            ++length;
        }

        /**
         * @brief delete the top element from the priority queue.
         * @throws container_is_empty if empty() returns true
         */
        void pop()
        {
            if (this->empty())
            {
                throw container_is_empty();
            }
            if (length == 1)
            {
                data[1].~T();
                length = 0;
                return;
            }
            T value = std::move(data[length]);
            data[length].~T();
            --length;

            size_t hole = 1;
            while (hole * 2 <= length)
            {
                size_t child = hole * 2;
                if (child < length && Compare()(data[child], data[child + 1]))
                {
                    ++child;
                }
                if (!Compare()(value, data[child]))
                {
                    break;
                }
                data[hole] = data[child];
                hole = child;
            }
            data[hole] = value;
        }

        /**
         * @brief return the number of elements in the priority queue.
         * @return the number of elements.
         */
        size_t size() const { return length; }

        /**
         * @brief check if the container is empty.
         * @return true if it is empty, false otherwise.
         */
        bool empty() const { return length == 0; }

        /**
         * @brief merge another priority_queue into this one.
         * The other priority_queue will be cleared after merging.
         * The complexity is at most O(logn).
         * @param other the priority_queue to be merged.
         */
        // void merge(priority_queue &other) {} this is an advanced function, you can choose to implement it or not. If
        // you don't want to implement it, please leave it as a comment.
    };

} // namespace sjtu

#endif
