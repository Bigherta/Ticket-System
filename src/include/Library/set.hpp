/**
 * implement a container like std::set
 */
#ifndef SJTU_SET_HPP
#define SJTU_SET_HPP

// only for std::less<T>
#include "exceptions.hpp"

namespace sjtu
{

    template<class Key, class Compare = std::less<Key>>
    class set
    {

    public:
        /**
         * the internal type of data.
         * it should have a default constructor, a copy constructor.
         * You can use sjtu::set as value_type by typedef.
         */
        Compare comp;
        typedef Key value_type;
        struct node
        {
            value_type key_value;
            node *left;
            node *right;
            node *parent;
            int height;
            node() : left(nullptr), right(nullptr), parent(nullptr), height(1) {}
            node(const value_type &key_value_, node *parent_ = nullptr, node *left_ = nullptr, node *right_ = nullptr) :
                key_value(key_value_), left(left_), right(right_), parent(parent_), height(1)
            {
            }
            ~node() = default;
        };
        node *root;
        node *leftmost;
        node *rightmost;
        std::size_t set_size;
        int height(node *Node) const { return Node ? Node->height : 0; }
        void update_height(node *Node)
        {
            if (Node)
                Node->height = std::max(height(Node->left), height(Node->right)) + 1;
        }
        int balance_factor(node *Node) const { return height(Node->left) - height(Node->right); }
        void rebalance_from(node *Node)
        {
            while (Node)
            {
                int old_height = Node->height;
                int bf = balance_factor(Node);
                if (bf > 1 || bf < -1)
                {
                    balance(Node);
                    Node = Node->parent;
                    continue;
                }

                update_height(Node);
                if (Node->height == old_height)
                    break;
                Node = Node->parent;
            }
        }
        node *clonetree(const node *other_root)
        {
            if (!other_root)
                return nullptr;
            node *new_root = new node(other_root->key_value);
            if (other_root->left)
            {
                new_root->left = clonetree(other_root->left);
                new_root->left->parent = new_root;
            }
            if (other_root->right)
            {
                new_root->right = clonetree(other_root->right);
                new_root->right->parent = new_root;
            }
            new_root->height = std::max(new_root->left ? new_root->left->height : 0,
                                        new_root->right ? new_root->right->height : 0) +
                               1;
            return new_root;
        }
        void cleartree(node *clear_root)
        {
            if (!clear_root)
                return;
            cleartree(clear_root->left);
            cleartree(clear_root->right);
            delete clear_root;
        }
        static node *next_node(const node *cur)
        {
            if (cur->right)
            {
                node *temp = const_cast<node *>(cur->right);
                while (temp->left)
                    temp = temp->left;
                return temp;
            }
            else
            {
                node *temp = const_cast<node *>(cur);
                while (temp->parent && temp->parent->right == temp)
                    temp = temp->parent;
                return temp->parent;
            }
            return nullptr;
        }
        static node *prev_node(const node *cur)
        {
            if (cur->left)
            {
                node *temp = const_cast<node *>(cur->left);
                while (temp->right)
                    temp = temp->right;
                return temp;
            }
            else
            {
                node *temp = const_cast<node *>(cur);
                while (temp->parent && temp->parent->left == temp)
                    temp = temp->parent;
                return temp->parent;
            }
        }
        static node *first_node(const node *root)
        {
            if (!root)
                return nullptr;
            node *cur = const_cast<node *>(root);
            while (cur->left)
                cur = cur->left;
            return cur;
        }
        static node *last_node(const node *root)
        {
            if (!root)
                return nullptr;
            node *cur = const_cast<node *>(root);
            while (cur->right)
                cur = cur->right;
            return cur;
        }
        void left_rotate(node *Node)
        {
            node *parent_ = Node->parent;
            node *new_parent_ = Node->right;
            Node->right = new_parent_->left;
            if (new_parent_->left)
                new_parent_->left->parent = Node;
            new_parent_->parent = parent_;
            if (!parent_)
                root = new_parent_;
            else if (parent_->left == Node)
                parent_->left = new_parent_;
            else
                parent_->right = new_parent_;
            new_parent_->left = Node;
            Node->parent = new_parent_;
            Node->height = std::max(height(Node->left), height(Node->right)) + 1;
            new_parent_->height = std::max(height(new_parent_->left), height(new_parent_->right)) + 1;
        }
        void right_rotate(node *Node)
        {
            node *parent_ = Node->parent;
            node *new_parent_ = Node->left;
            Node->left = new_parent_->right;
            if (new_parent_->right)
                new_parent_->right->parent = Node;
            new_parent_->parent = parent_;
            if (!parent_)
                root = new_parent_;
            else if (parent_->left == Node)
                parent_->left = new_parent_;
            else
                parent_->right = new_parent_;
            new_parent_->right = Node;
            Node->parent = new_parent_;
            Node->height = std::max(height(Node->left), height(Node->right)) + 1;
            new_parent_->height = std::max(height(new_parent_->left), height(new_parent_->right)) + 1;
        }
        void balance(node *Node)
        {
            switch (balance_factor(Node))
            {
                case 2: {
                    int left_bf = balance_factor(Node->left);
                    if (left_bf >= 0)
                    {
                        right_rotate(Node);
                    }
                    else
                    {
                        left_rotate(Node->left);
                        right_rotate(Node);
                    }
                    break;
                }
                case -2: {
                    int right_bf = balance_factor(Node->right);
                    if (right_bf <= 0)
                    {
                        left_rotate(Node);
                    }
                    else
                    {
                        right_rotate(Node->right);
                        left_rotate(Node);
                    }
                    break;
                }
                default:
                    break;
            }
        }
        /**
         * see BidirectionalIterator at CppReference for help.
         *
         * if there is anything wrong throw invalid_iterator.
         *     like it = set.begin(); --it;
         *       or it = set.end(); ++end();
         */
        class iterator
        {
        private:
            /**
             * TODO add data members
             *   just add whatever you want.
             */
            node *data;
            const set *parent;

        public:
            iterator()
            {
                // TODO
                data = nullptr;
                parent = nullptr;
            }

            iterator(const iterator &other)
            {
                // TODO
                data = other.data;
                parent = other.parent;
            }

            iterator(node *Node_, const set *parent_)
            {
                data = Node_;
                parent = parent_;
            }
            /**
             * TODO iter++
             */
            iterator operator++(int)
            {
                if (!parent)
                    throw invalid_iterator();
                auto temp = *this;
                if (data)
                    data = next_node(data);
                return temp;
            }

            /**
             * TODO ++iter
             */
            iterator &operator++()
            {
                if (!parent)
                    throw invalid_iterator();
                if (data)
                    data = next_node(data);
                return *this;
            }

            /**
             * TODO iter--
             */
            iterator operator--(int)
            {
                if (!parent)
                    throw invalid_iterator();
                auto temp = *this;
                if (!data)
                {
                    if (!parent->rightmost)
                        throw invalid_iterator();
                    data = parent->rightmost;
                    return temp;
                }
                if (data == parent->leftmost)
                    return temp;
                data = prev_node(data);
                return temp;
            }

            /**
             * TODO --iter
             */
            iterator &operator--()
            {
                if (!parent)
                    throw invalid_iterator();
                if (!data)
                {
                    if (!parent->rightmost)
                        throw invalid_iterator();
                    data = parent->rightmost;
                    return *this;
                }
                if (data == parent->leftmost)
                    return *this;
                data = prev_node(data);
                return *this;
            }

            const value_type &operator*() const
            {
                if (!data)
                    throw invalid_iterator();
                return data->key_value;
            }
            node *get_node() const { return data; }
            const set *get_parent() const { return parent; }
            /**
             * a operator to check whether two iterators are same (pointing to the same memory).
             */
            bool operator==(const iterator &rhs) const { return data == rhs.data && parent == rhs.parent; }

            bool operator!=(const iterator &rhs) const { return data != rhs.data || parent != rhs.parent; }

            const value_type *operator->() const noexcept { return &data->key_value; }
        };
        /**
         * TODO two constructors
         */
        set()
        {
            root = nullptr;
            leftmost = nullptr;
            rightmost = nullptr;
            set_size = 0;
        }

        set(const set &other)
        {
            comp = other.comp;
            root = clonetree(other.root);
            leftmost = first_node(root);
            rightmost = last_node(root);
            set_size = other.set_size;
        }
        set(set &&other) noexcept
        {
            comp = std::move(other.comp);
            root = other.root;
            leftmost = other.leftmost;
            rightmost = other.rightmost;
            set_size = other.set_size;
            other.root = nullptr;
            other.leftmost = nullptr;
            other.rightmost = nullptr;
            other.set_size = 0;
        }
        /**
         * TODO assignment operator
         */
        set &operator=(const set &other)
        {
            if (this == &other)
                return *this;
            cleartree(root);
            comp = other.comp;
            root = clonetree(other.root);
            leftmost = first_node(root);
            rightmost = last_node(root);
            set_size = other.set_size;
            return *this;
        }

        set &operator=(set &&other) noexcept
        {
            if (this == &other)
                return *this;
            cleartree(root);
            comp = std::move(other.comp);
            root = other.root;
            leftmost = other.leftmost;
            rightmost = other.rightmost;
            set_size = other.set_size;
            other.root = nullptr;
            other.leftmost = nullptr;
            other.rightmost = nullptr;
            other.set_size = 0;
            return *this;
        }

        /**
         * TODO Destructors
         */
        ~set() { cleartree(root); }

        /**
         * return a iterator to the beginning
         */
        iterator begin() const noexcept { return iterator(leftmost, this); }

        /**
         * return a iterator to the end
         * in fact, it returns past-the-end.
         */
        iterator end() const noexcept { return iterator(nullptr, this); }
        /**
         * returns the number of elements.
         */
        size_t size() const noexcept { return set_size; }

        /**
         * clears the contents
         */
        void clear()
        {
            cleartree(root);
            root = nullptr;
            leftmost = nullptr;
            rightmost = nullptr;
            set_size = 0;
        }

        /**
         * insert an element.
         * return a pair, the first of the pair is
         *   the iterator to the new element (or the element that prevented the insertion),
         *   the second one is true if insert successfully, or false.
         */
        template<class... Args>
        std::pair<iterator, bool> emplace(Args &&...args)
        {
            value_type value(std::forward<Args>(args)...);
            if (root == nullptr)
            {
                root = new node(value);
                leftmost = root;
                rightmost = root;
                set_size++;
                return {iterator(root, this), true};
            }
            node *cur = root;
            node *parent_ = nullptr;
            while (cur)
            {
                parent_ = cur;
                if (comp(value, cur->key_value))
                    cur = cur->left;
                else if (comp(cur->key_value, value))
                    cur = cur->right;
                else
                {
                    return {iterator(cur, this), false};
                }
            }
            set_size++;
            cur = new node(value, parent_);
            const bool is_left_child = comp(value, parent_->key_value);
            if (is_left_child)
                parent_->left = cur;
            else
                parent_->right = cur;
            if (!leftmost || comp(cur->key_value, leftmost->key_value))
                leftmost = cur;
            if (!rightmost || comp(rightmost->key_value, cur->key_value))
                rightmost = cur;
            iterator result(cur, this);

            rebalance_from(parent_);
            return {result, true};
        }

        /**
         * erase the element at pos.
         *
         * throw if pos pointed to a bad element (pos == this->end() || pos points an element out of this)
         */
        size_t erase(iterator pos)
        {
            if (pos == end() || pos.get_parent() != this)
                throw invalid_iterator();
            node *cur = pos.get_node();
            const bool erase_leftmost = (cur == leftmost);
            const bool erase_rightmost = (cur == rightmost);
            node *parent_ = cur->parent;
            node *start_balance_node = parent_;
            // case 1: no child
            if (!cur->left && !cur->right)
            {
                if (!parent_)
                {
                    root = nullptr;
                }
                else if (parent_->left == cur)
                    parent_->left = nullptr;
                else
                    parent_->right = nullptr;
                delete cur;
                set_size--;
            }
            else
            {
                // case 2: one child
                if (!cur->left || !cur->right)
                {
                    node *child = cur->left ? cur->left : cur->right;
                    if (!parent_)
                    {
                        root = child;
                        child->parent = nullptr;
                    }
                    else if (parent_->left == cur)
                    {
                        parent_->left = child;
                        child->parent = parent_;
                    }
                    else
                    {
                        parent_->right = child;
                        child->parent = parent_;
                    }
                    delete cur;
                    set_size--;
                }
                // case 3: two children
                else
                {
                    node *successor = next_node(cur);
                    node *successor_parent = successor->parent;
                    node *successor_child = successor->right;
                    node *left_child = cur->left;
                    node *right_child = cur->right;

                    // pick the successor off the tree
                    if (successor_parent != cur)
                    {
                        start_balance_node = successor_parent;
                        successor_parent->left = successor_child;
                        if (successor_child)
                            successor_child->parent = successor_parent;
                    }
                    else
                    {
                        start_balance_node = successor;
                        successor_parent->right = successor_child;
                        if (successor_child)
                            successor_child->parent = successor_parent;
                    }

                    // replace cur with successor
                    successor->parent = parent_;
                    if (!parent_)
                    {
                        root = successor;
                    }
                    else if (parent_->left == cur)
                        parent_->left = successor;
                    else
                        parent_->right = successor;

                    // replace successor's left and right child with cur's left and right child
                    successor->left = left_child;
                    if (left_child)
                        left_child->parent = successor;
                    if (successor_parent != cur)
                    {
                        successor->right = right_child;
                        if (right_child)
                            right_child->parent = successor;
                    }
                    else
                    {
                        successor->right = successor_child;
                        if (successor_child)
                            successor_child->parent = successor;
                    }
                    delete cur;
                    set_size--;
                }
            }
            if (set_size == 0)
            {
                leftmost = nullptr;
                rightmost = nullptr;
            }
            else
            {
                if (erase_leftmost)
                    leftmost = first_node(root);
                if (erase_rightmost)
                    rightmost = last_node(root);
            }
            while (start_balance_node)
            {
                int old_height = start_balance_node->height;
                int bf = balance_factor(start_balance_node);
                if (bf > 1 || bf < -1)
                {
                    balance(start_balance_node);
                    start_balance_node = start_balance_node->parent;
                    continue;
                }
                update_height(start_balance_node);
                if (start_balance_node->height == old_height)
                    break;
                start_balance_node = start_balance_node->parent;
            }
            return 1;
        }

        /**
         * Finds an element with key equivalent to key.
         * key value of the element to search for.
         * Iterator to an element with key equivalent to key.
         *   If no such element is found, past-the-end (see end()) iterator is returned.
         */
        iterator find(const Key &key) const
        {
            node *cur = root;
            while (cur)
            {
                if (comp(key, cur->key_value))
                    cur = cur->left;
                else if (comp(cur->key_value, key))
                    cur = cur->right;
                else
                    return iterator(cur, this);
            }
            return end();
        }

        size_t erase(const Key &key)
        {
            node *cur = root;
            while (cur)
            {
                if (comp(key, cur->key_value))
                    cur = cur->left;
                else if (comp(cur->key_value, key))
                    cur = cur->right;
                else
                {
                    iterator it(cur, this);
                    erase(it);
                    return 1;
                }
            }
            return 0;
        }

        iterator lower_bound(const Key &key) const
        {
            node *cur = root;
            node *ans = nullptr;
            while (cur)
            {
                if (!comp(cur->key_value, key))
                {
                    ans = cur;
                    cur = cur->left;
                }
                else
                    cur = cur->right;
            }
            if (ans)
                return iterator(ans, this);
            return end();
        }

        iterator upper_bound(const Key &key) const
        {
            node *cur = root;
            node *ans = nullptr;
            while (cur)
            {
                if (comp(key, cur->key_value))
                {
                    ans = cur;
                    cur = cur->left;
                }
                else
                    cur = cur->right;
            }
            if (ans)
                return iterator(ans, this);
            return end();
        }

        size_t range(const Key &l, const Key &r) const
        {
            if (comp(r, l))
                return 0;
            size_t cnt = 0;
            for (iterator it = lower_bound(l); it != end(); ++it)
            {
                if (comp(r, *it))
                    break;
                ++cnt;
            }
            return cnt;
        }
    };

} // namespace sjtu
template <typename T>
using set = sjtu::set<T>;
#endif
