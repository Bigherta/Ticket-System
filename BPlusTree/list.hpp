#pragma once
#ifndef SJTU_LIST_HPP
#define SJTU_LIST_HPP

#include <cstddef>
#include <new>

namespace sjtu
{


    /**
     * @tparam T Type of the elements.
     * Be careful that T may not be default constructable.
     *
     * @brief A list that supports operations like std::list.
     *
     * We encourage you to design the implementation yourself.
     * As for the memory management, you may use std::allocator,
     * new/delete, malloc/free or something else.
     */
    template<typename T>
    class list
    {
    public:
        class iterator;
        class const_iterator;
        struct node
        {
            T *value;
            node *prev, *next;
            node()
            {
                value = nullptr;
                prev = next = nullptr;
            }
            ~node() = default;
        };

    private:
        node *create_node(const T &value)
        {
            void *raw = operator new(sizeof(node) + sizeof(T));
            node *new_node = new (raw) node();
            new_node->value = reinterpret_cast<T *>(static_cast<char *>(raw) + sizeof(node));
            new (new_node->value) T(value);
            return new_node;
        }

        void destroy_node(node *temp) noexcept
        {
            if (!temp)
                return;
            if (temp->value)
            {
                temp->value->~T();
                temp->value = nullptr;
            }
            temp->~node();
            operator delete(temp);
        }

    public:
        void insert(node *first, node *second, const T &value)
        {
            node *new_node = create_node(value);
            new_node->prev = first;
            new_node->next = second;
            first->next = new_node;
            second->prev = new_node;
            length++;
        }
        void erase(node *temp)
        {
            if (temp == head || temp == tail)
                return;
            node *first = temp->prev, *second = temp->next;
            first->next = second;
            second->prev = first;
            temp->prev = temp->next = nullptr;
            destroy_node(temp);
            length--;
        }
        node *head, *tail;
        size_t length;

    public:
        /**
         * Constructs & Assignments
         * At least three: default constructor, copy constructor/assignment
         * Bonus: move/initializer_list constructor/assignment
         */
        list()
        {
            head = new node();
            tail = new node();
            head->next = tail;
            tail->prev = head;
            length = 0;
        }
        list(const list &other)
        {
            head = new node();
            tail = new node();
            head->next = tail;
            tail->prev = head;
            length = 0;
            for (node *cur = other.head->next; cur != other.tail; cur = cur->next)
                push_back(*(cur->value));
        }
        list &operator=(const list &other)
        {
            if (this != &other)
            {
                clear();
                for (node *cur = other.head->next; cur != other.tail; cur = cur->next)
                    push_back(*(cur->value));
            }
            return *this;
        }

        /* Destructor. */
        ~list()
        {
            clear();
            delete head;
            delete tail;
        }

        /* Access the first / last element. */
        T &front() noexcept { return *(head->next->value); }
        T &back() noexcept { return *(tail->prev->value); }
        const T &front() const noexcept { return *(head->next->value); }
        const T &back() const noexcept { return *(tail->prev->value); }

        /* Return an iterator pointing to the first element. */
        iterator begin() noexcept { return iterator{head->next}; }
        const_iterator cbegin() const noexcept { return const_iterator{head->next}; }

        /* Return an iterator pointing to one past the last element. */
        iterator end() noexcept { return iterator{tail}; }
        const_iterator cend() const noexcept { return const_iterator{tail}; }

        /* Checks whether the container is empty. */
        bool empty() const noexcept { return length == 0; }
        /* Return count of elements in the container. */
        size_t size() const noexcept { return length; }

        /* Clear the contents. */
        void clear() noexcept
        {
            node *cur = head->next;
            while (cur != tail)
            {
                node *next_node = cur->next;
                destroy_node(cur);
                cur = next_node;
            }
            head->next = tail;
            tail->prev = head;
            length = 0;
        }

        /**
         * @brief Insert value before pos (pos may be the end() iterator).
         * @return An iterator pointing to the inserted value.
         * @throw Throw if the iterator is invalid.
         */
        iterator insert(iterator pos, const T &value)
        {
            node *second = pos.ptr;
            node *first = second->prev;
            insert(first, second, value);
            return iterator{first->next};
        }

        /**
         * @brief Remove the element at pos (remove end() iterator is invalid).
         * returns an iterator pointing to the following element, if pos pointing to
         * the last element, end() will be returned.
         * @throw Throw if the container is empty, or the iterator is invalid.
         */
        iterator erase(iterator pos) noexcept
        {
            node *temp = pos.ptr;
            node *second = temp->next;
            erase(temp);
            return iterator{second};
        }

        void splice(iterator pos, list &other, iterator it)
        {
            if (pos == it)
                return;
            iterator next_it = it;
            ++next_it;
            if (pos == next_it)
                return;

            node *other_node = it.ptr;
            node *other_node_prev = other_node->prev;
            node *other_node_next = other_node->next;
            other_node_next->prev = other_node_prev;
            other_node_prev->next = other_node_next;

            node *pos_node = pos.ptr;
            node *pos_node_prev = pos_node->prev;
            pos_node_prev->next = other_node;
            other_node->prev = pos_node_prev;
            other_node->next = pos_node;
            pos_node->prev = other_node;
            
            if (&other != this)
            {
                length++;
                other.length--;
            }
        }

        /* Add an element to the front/back. */
        void push_front(const T &value) { insert(head, head->next, value); }
        void push_back(const T &value) { insert(tail->prev, tail, value); }

        /* Removes the first/last element. */
        void pop_front() noexcept { erase(head->next); }
        void pop_back() noexcept { erase(tail->prev); }

    public:
        /**
         * Basic requirements:
         * operator ++, --, *, ->
         * operator ==, != between iterators and const iterators
         * constructing a const iterator from an iterator
         *
         * If your implementation meets these requirements,
         * you may not comply with the following template.
         * You may even move this template outside the class body,
         * as long as your code works well.
         *
         * Contact TA whenever you are not sure.
         */
        class iterator
        {
        public:
            /**
             * TODO just add whatever you want.
             */
            node *ptr;

        public:
            iterator() : ptr(nullptr) {}
            iterator(node *p) : ptr(p) {}
            iterator &operator++()
            {
                if (ptr->next)
                    ptr = ptr->next;
                return *this;
            }
            iterator &operator--()
            {
                if (ptr->prev)
                    ptr = ptr->prev;
                return *this;
            }
            iterator operator++(int)
            {
                iterator temp = *this;
                ++(*this);
                return temp;
            }
            iterator operator--(int)
            {
                iterator temp = *this;
                --(*this);
                return temp;
            }

            T &operator*() const noexcept { return *(ptr->value); }
            T *operator->() const noexcept { return ptr->value; }

            /* A operator to check whether two iterators are same (pointing to the same memory) */
            friend bool operator==(const iterator &a, const iterator &b) { return a.ptr == b.ptr; }
            friend bool operator!=(const iterator &a, const iterator &b) { return a.ptr != b.ptr; }
        };

        /**
         * Const iterator should have same functions as iterator, just for a const object.
         * It should be able to construct from an iterator.
         * It should be able to compare with an iterator.
         */
        class const_iterator
        {
        public:
            node *ptr;

        public:
            const_iterator() : ptr(nullptr) {}
            const_iterator(node *p) : ptr(p) {}
            const_iterator(const iterator &other) : ptr(other.ptr) {}
            const_iterator &operator++()
            {
                if (ptr->next)
                    ptr = ptr->next;
                return *this;
            }
            const_iterator &operator--()
            {
                if (ptr->prev)
                    ptr = ptr->prev;
                return *this;
            }
            const_iterator operator++(int)
            {
                const_iterator temp = *this;
                ++(*this);
                return temp;
            }
            const_iterator operator--(int)
            {
                const_iterator temp = *this;
                --(*this);
                return temp;
            }
            const T &operator*() const noexcept { return *(ptr->value); }
            const T *operator->() const noexcept { return ptr->value; }
            friend bool operator==(const const_iterator &a, const const_iterator &b) { return a.ptr == b.ptr; }
            friend bool operator!=(const const_iterator &a, const const_iterator &b) { return a.ptr != b.ptr; }
            friend bool operator==(const const_iterator &a, const iterator &b) { return a.ptr == b.ptr; }
            friend bool operator!=(const const_iterator &a, const iterator &b) { return a.ptr != b.ptr; }
            friend bool operator==(const iterator &a, const const_iterator &b) { return a.ptr == b.ptr; }
            friend bool operator!=(const iterator &a, const const_iterator &b) { return a.ptr != b.ptr; }
        };
    };

} // namespace sjtu

#endif // SJTU_LIST_HPP
