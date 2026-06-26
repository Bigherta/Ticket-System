/**
 * implement a container like std::set
 */
#ifndef SJTU_SET_HPP
#define SJTU_SET_HPP

// only for std::less<T>
#include "utility.hpp"
#include "exceptions.hpp"
namespace sjtu
{

    template<class Key, class Compare = std::less<Key>>
    class set
    {

    private:
        enum class Color
        {
            RED,
            BLACK
        };
        Compare comp;
        struct node_base
        {
            node_base *left;
            node_base *right;
            node_base *parent;
            Color color;
            node_base(Color color_ = Color::BLACK, node_base *parent_ = nullptr, node_base *left_ = nullptr,
                      node_base *right_ = nullptr) : color(color_), parent(parent_), left(left_), right(right_)
            {
            }
        };
        struct node : public node_base
        {
            Key key_value;
            node(const Key &key_value_, Color color_ = Color::RED, node *parent_ = nullptr,
                 node *left_ = nullptr, node *right_ = nullptr) :
                node_base(color_, parent_, left_, right_), key_value(key_value_)
            {
            }
            ~node() = default;
        };
        node_base *root;
        node_base *Begin;
        node_base *End;
        std::size_t set_size;
        node *clonetree(const node *other_root)
        {
            if (!other_root)
                return nullptr;
            node *new_root = new node(other_root->key_value, other_root->color);
            if (other_root->left)
            {
                new_root->left = clonetree(static_cast<const node *>(other_root->left));
                new_root->left->parent = new_root;
            }
            if (other_root->right)
            {
                new_root->right = clonetree(static_cast<const node *>(other_root->right));
                new_root->right->parent = new_root;
            }
            return new_root;
        }
        void cleartree(node *clear_root)
        {
            if (!clear_root)
                return;
            cleartree(static_cast<node *>(clear_root->left));
            cleartree(static_cast<node *>(clear_root->right));
            delete clear_root;
        }
        static node *next_node(const node_base *cur)
        {
            if (cur->right)
            {
                node_base *temp = const_cast<node_base *>(cur->right);
                while (temp->left)
                    temp = temp->left;
                return static_cast<node *>(temp);
            }
            else
            {
                node_base *temp = const_cast<node_base *>(cur);
                while (temp->parent && temp->parent->right == temp)
                    temp = temp->parent;
                return static_cast<node *>(temp->parent);
            }
        }
        static node *prev_node(const node_base *cur)
        {
            if (cur->left)
            {
                node_base *temp = const_cast<node_base *>(cur->left);
                while (temp->right)
                    temp = temp->right;
                return static_cast<node *>(temp);
            }
            else
            {
                node_base *temp = const_cast<node_base *>(cur);
                while (temp->parent && temp->parent->left == temp)
                    temp = temp->parent;
                return static_cast<node *>(temp->parent);
            }
        }
        static node *first_node(const node_base *rt)
        {
            if (!rt)
                return nullptr;
            node_base *cur = const_cast<node_base *>(rt);
            while (cur->left)
                cur = cur->left;
            return static_cast<node *>(cur);
        }
        static node *last_node(const node_base *rt)
        {
            if (!rt)
                return nullptr;
            node_base *cur = const_cast<node_base *>(rt);
            while (cur->right)
                cur = cur->right;
            return static_cast<node *>(cur);
        }
        void left_rotate(node_base *Node)
        {
            node_base *parent_ = Node->parent;
            node_base *new_parent_ = Node->right;
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
        }
        void right_rotate(node_base *Node)
        {
            node_base *parent_ = Node->parent;
            node_base *new_parent_ = Node->left;
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
        }

        void insert_rebalance(node_base *Node)
        {
            if (!Node)
                return;
            if (!Node->parent)
            {
                Node->color = Color::BLACK;
                return;
            }
            if (Node->parent->color == Color::BLACK)
                return;
            node_base *parent_ = Node->parent;
            node_base *grandparent_ = parent_->parent;
            if (!grandparent_)
            {
                parent_->color = Color::BLACK;
                return;
            }
            node_base *uncle_ = (grandparent_->left == parent_) ? grandparent_->right : grandparent_->left;
            if (uncle_ && uncle_->color == Color::RED)
            {
                parent_->color = Color::BLACK;
                uncle_->color = Color::BLACK;
                grandparent_->color = Color::RED;
                insert_rebalance(grandparent_);
            }
            else
            {
                if (grandparent_->left == parent_)
                {
                    if (parent_->right == Node)
                    {
                        left_rotate(parent_);
                        Node = parent_;
                        parent_ = static_cast<node_base *>(Node->parent);
                    }
                    right_rotate(grandparent_);
                }
                else
                {
                    if (parent_->left == Node)
                    {
                        right_rotate(parent_);
                        Node = parent_;
                        parent_ = static_cast<node_base *>(Node->parent);
                    }
                    left_rotate(grandparent_);
                }
                parent_->color = Color::BLACK;
                grandparent_->color = Color::RED;
            }
        }

        void delete_rebalance(node_base *Node)
        {
            if (!Node)
                return;
            while (Node != root)
            {
                node_base *parent_ = Node->parent;
                node_base *sibling_ = (parent_->left == Node) ? parent_->right : parent_->left;

                // null sibling is implicitly BLACK — treat as "sibling black with both children black"
                if (!sibling_)
                {
                    if (parent_->color == Color::RED)
                    {
                        parent_->color = Color::BLACK;
                        break;
                    }
                    else
                    {
                        Node = parent_;
                        continue;
                    }
                }

                if (sibling_->color == Color::RED)
                {
                    sibling_->color = Color::BLACK;
                    parent_->color = Color::RED;
                    if (parent_->left == Node)
                        left_rotate(parent_);
                    else
                        right_rotate(parent_);
                    continue;
                }
                else
                {
                    bool is_sibling_left_red = sibling_->left && sibling_->left->color == Color::RED;
                    bool is_sibling_right_red = sibling_->right && sibling_->right->color == Color::RED;
                    if (!is_sibling_left_red && !is_sibling_right_red)
                    {
                        sibling_->color = Color::RED;
                        if (parent_->color == Color::RED)
                        {
                            parent_->color = Color::BLACK;
                            break;
                        }
                        else
                        {
                            Node = parent_;
                            continue;
                        }
                    }
                    bool is_sibling_left = parent_->left == sibling_;
                    if (is_sibling_left && is_sibling_left_red)
                    {
                        sibling_->left->color = sibling_->color;
                        sibling_->color = parent_->color;
                        parent_->color = Color::BLACK;
                        right_rotate(parent_);
                        break;
                    }
                    else if (!is_sibling_left && is_sibling_right_red)
                    {
                        sibling_->right->color = sibling_->color;
                        sibling_->color = parent_->color;
                        parent_->color = Color::BLACK;
                        left_rotate(parent_);
                        break;
                    }
                    else if (is_sibling_left && is_sibling_right_red)
                    {
                        sibling_->right->color = parent_->color;
                        parent_->color = Color::BLACK;
                        left_rotate(sibling_);
                        right_rotate(parent_);
                        break;
                    }
                    else if (!is_sibling_left && is_sibling_left_red)
                    {
                        sibling_->left->color = parent_->color;
                        parent_->color = Color::BLACK;
                        right_rotate(sibling_);
                        left_rotate(parent_);
                        break;
                    }
                }
            }
            root->color = Color::BLACK;
        }

        /**
         * Replace subtree rooted at u with subtree rooted at v.
         * u's parent becomes v's parent; u is not freed.
         */
        void transplant(node_base *u, node_base *v)
        {
            if (!u->parent)
                root = v;
            else if (u == u->parent->left)
                u->parent->left = v;
            else
                u->parent->right = v;
            if (v)
                v->parent = u->parent;
        }
        public:
        /**
         * see BidirectionalIterator at CppReference for help.
         *
         * if there is anything wrong throw std::runtime_error.
         *     like it = set.begin(); --it;
         *       or it = set.end(); ++it;
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
                    if (!parent->End)
                        throw invalid_iterator();
                    data = static_cast<node*>(parent->End);
                    return temp;
                }
                if (data == parent->Begin)
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
                    if (!parent->End)
                        throw invalid_iterator();
                    data = static_cast<node*>(parent->End);
                    return *this;
                }
                if (data == parent->Begin)
                    return *this;
                data = prev_node(data);
                return *this;
            }

            const Key &operator*() const
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

            const Key *operator->() const noexcept { return &data->key_value; }
        };
        /**
         * TODO two constructors
         */
        set()
        {
            root = nullptr;
            Begin = nullptr;
            End = nullptr;
            set_size = 0;
        }

        set(const set &other)
        {
            comp = other.comp;
            root = clonetree(static_cast<const node*>(other.root));
            Begin = first_node(root);
            End = last_node(root);
            set_size = other.set_size;
        }
        set(set &&other) noexcept
        {
            comp = std::move(other.comp);
            root = other.root;
            Begin = other.Begin;
            End = other.End;
            set_size = other.set_size;
            other.root = nullptr;
            other.Begin = nullptr;
            other.End = nullptr;
            other.set_size = 0;
        }
        /**
         * TODO assignment operator
         */
        set &operator=(const set &other)
        {
            if (this == &other)
                return *this;
            cleartree(static_cast<node*>(root));
            comp = other.comp;
            root = clonetree(static_cast<const node*>(other.root));
            Begin = first_node(root);
            End = last_node(root);
            set_size = other.set_size;
            return *this;
        }

        set &operator=(set &&other) noexcept
        {
            if (this == &other)
                return *this;
            cleartree(static_cast<node*>(root));
            comp = std::move(other.comp);
            root = other.root;
            Begin = other.Begin;
            End = other.End;
            set_size = other.set_size;
            other.root = nullptr;
            other.Begin = nullptr;
            other.End = nullptr;
            other.set_size = 0;
            return *this;
        }

        /**
         * TODO Destructors
         */
        ~set() { cleartree(static_cast<node*>(root)); }

        /**
         * return a iterator to the beginning
         */
        iterator begin() const noexcept { return iterator(static_cast<node*>(Begin), this); }

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
            cleartree(static_cast<node*>(root));
            root = nullptr;
            Begin = nullptr;
            End = nullptr;
            set_size = 0;
        }

        /**
         * insert an element.
         * return a pair, the first of the pair is
         *   the iterator to the new element (or the element that prevented the insertion),
         *   the second one is true if insert successfully, or false.
         */
        template<class... Args>
        sjtu::pair<iterator, bool> emplace(Args &&...args)
        {
            Key value(std::forward<Args>(args)...);
            if (root == nullptr)
            {
                root = new node(value, Color::BLACK);
                Begin = root;
                End = root;
                set_size++;
                return {iterator(static_cast<node*>(root), this), true};
            }
            node *cur = static_cast<node*>(root);
            node *parent_ = nullptr;
            while (cur)
            {
                parent_ = cur;
                if (comp(value, cur->key_value))
                    cur = static_cast<node*>(cur->left);
                else if (comp(cur->key_value, value))
                    cur = static_cast<node*>(cur->right);
                else
                {
                    return {iterator(cur, this), false};
                }
            }
            set_size++;
            node *new_node = new node(value, Color::RED, parent_);
            const bool is_left_child = comp(value, parent_->key_value);
            if (is_left_child)
                parent_->left = new_node;
            else
                parent_->right = new_node;
            if (comp(new_node->key_value, static_cast<node*>(Begin)->key_value))
                Begin = new_node;
            if (comp(static_cast<node*>(End)->key_value, new_node->key_value))
                End = new_node;
            iterator result(new_node, this);
            insert_rebalance(new_node);
            return {result, true};
        }

        /**
         * erase the element at pos.
         *
         * throw if pos pointed to a bad element (pos == this->end() || pos points an element out of this)
         */
        size_t erase(iterator pos)
        {
            if (pos.get_parent() != this || pos.get_node() == nullptr)
                throw invalid_iterator();

            node *target = pos.get_node();

            // ── Determine which node will be physically removed from the tree ──
            // If target has ≤1 child, target itself is removed.
            // If target has two children, its successor is physically extracted.
            node *removed;
            if (target->left == nullptr || target->right == nullptr)
                removed = target;
            else
                removed = first_node(target->right);

            // ── Save removed-node position for fixup when replacement is null ──
            node_base *fix_parent = removed->parent;
            bool fix_is_left = fix_parent && (fix_parent->left == removed);

            // ── Standard RB-tree deletion (CLRS Chapter 13) ──
            node_base *replacement; // child that replaces 'removed' in the tree
            Color removed_color;

            if (target->left == nullptr)
            {
                replacement = target->right;
                transplant(target, target->right);
                removed_color = target->color;
            }
            else if (target->right == nullptr)
            {
                replacement = target->left;
                transplant(target, target->left);
                removed_color = target->color;
            }
            else
            {
                // Two children: successor takes target's place, no key copy needed
                node *successor = removed; // = first_node(target->right)
                removed_color = successor->color;
                replacement = successor->right;

                // Save successor's original parent / side BEFORE any transplant
                fix_parent = successor->parent;
                fix_is_left = (fix_parent->left == successor);

                if (successor->parent == target)
                {
                    if (replacement)
                        replacement->parent = successor;
                    // After transplant(target, successor), the null at successor->right
                    // has parent = successor (now at target's old position), side = right
                    fix_parent = successor;
                    fix_is_left = false;
                }
                else
                {
                    transplant(successor, successor->right);
                    successor->right = target->right;
                    successor->right->parent = successor;
                }
                transplant(target, successor);
                successor->left = target->left;
                successor->left->parent = successor;
                successor->color = target->color;
            }

            // ── Fixup ──
            if (removed_color == Color::BLACK)
                delete_rebalance(replacement);

            // ── Save whether Begin/End are being removed (compare before delete) ──
            bool removing_begin = (removed == Begin);
            bool removing_end   = (removed == End);

            delete target;
            --set_size;

            if (set_size == 0)
            {
                Begin = nullptr;
                End   = nullptr;
            }
            else
            {
                if (removing_begin)
                    Begin = first_node(root);
                if (removing_end)
                    End   = last_node(root);
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
            node *cur = static_cast<node*>(root);
            while (cur)
            {
                if (comp(key, cur->key_value))
                    cur = static_cast<node*>(cur->left);
                else if (comp(cur->key_value, key))
                    cur = static_cast<node*>(cur->right);
                else
                    return iterator(cur, this);
            }
            return end();
        }

        size_t erase(const Key &key)
        {
            node *cur = static_cast<node*>(root);
            while (cur)
            {
                if (comp(key, cur->key_value))
                    cur = static_cast<node*>(cur->left);
                else if (comp(cur->key_value, key))
                    cur = static_cast<node*>(cur->right);
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
            node *cur = static_cast<node*>(root);
            node *ans = nullptr;
            while (cur)
            {
                if (!comp(cur->key_value, key))
                {
                    ans = cur;
                    cur = static_cast<node*>(cur->left);
                }
                else
                    cur = static_cast<node*>(cur->right);
            }
            if (ans)
                return iterator(ans, this);
            return end();
        }

        iterator upper_bound(const Key &key) const
        {
            node *cur = static_cast<node*>(root);
            node *ans = nullptr;
            while (cur)
            {
                if (comp(key, cur->key_value))
                {
                    ans = cur;
                    cur = static_cast<node*>(cur->left);
                }
                else
                    cur = static_cast<node*>(cur->right);
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
template <typename T, typename Compare = std::less<T>>
using set = sjtu::set<T, Compare>;
#endif
