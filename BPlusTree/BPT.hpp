#ifndef BPT_HPP
#define BPT_HPP
#include <iostream>
#include "BPT_MemoryRiver.hpp"
#include "utility.hpp"
#include "vector.hpp"
constexpr int order = 128;
template<class T>
int BinarySearch(const T arr[], int size, const T &key) // upper_bound
{
    int left = 0, right = size;
    while (left < right)
    {
        int mid = left + (right - left) / 2;
        if (arr[mid] <= key)
            left = mid + 1;
        else
            right = mid;
    }
    return left;
}
template<class T>
int BinarySearch(const sjtu::pair<T, int> arr[], int size, const T &key) // lower_bound
{
    int left = 0, right = size;
    while (left < right)
    {
        int mid = left + (right - left) / 2;
        if (arr[mid].first < key)
            left = mid + 1;
        else
            right = mid;
    }
    return left;
}
template<class T>
class BPT
{
private:
    // Standard definition: order = maximum children count of an internal node.
    static constexpr int max_children = order;
    static constexpr int max_keys = max_children - 1;
    static constexpr int min_children_non_root_internal = (max_children + 1) / 2; // ceil(m / 2)
    static constexpr int min_internal_keys_non_root = min_children_non_root_internal - 1; // ceil(m / 2) - 1
    static constexpr int min_leaf_keys_non_root = (max_keys + 1) / 2; // ceil((m - 1) / 2)

    template<int M>
    struct Node
    {
        bool isLeaf; // true for leaf, false for internal
        int size; // number of keys currently in the node
        sjtu::pair<T, int> Keys[M]; // keys and values in the node
        int parent; // index of the parent node
        int children[M + 1]; // index, internal only
        int next; // leaf only, index of the next leaf node
        Node()
        {
            isLeaf = false;
            size = 0;
            parent = -1;
            next = -1;
        }
    };
    int root_pos = 2 * sizeof(int); // position of the root node in the file
    int tree_size = 0; // number of nodes in the tree
    MemoryRiver<Node<order>> BPTree; // B+ tree with order 5
    void split(Node<order> &node, int node_pos)
    {
        int left_size = order / 2; // number of keys to keep in the left node
        int right_size = node.size - left_size;
        int new_node_pos;
        bool is_parent_split = false;
        // Create new node and move the right half of the keys to it
        Node<order> newNode;
        newNode.isLeaf = node.isLeaf;
        newNode.parent = node.parent;
        if (node.isLeaf)
        {
            newNode.size = right_size;
            for (int i = 0; i < right_size; ++i)
                newNode.Keys[i] = node.Keys[left_size + i];
            newNode.next = node.next;
            new_node_pos = BPTree.write(newNode);
            ++tree_size;
            node.next = new_node_pos;
        }
        else
        {
            newNode.size = right_size - 1; // the middle key will be moved up to the parent node
            for (int i = 1; i < right_size; ++i)
                newNode.Keys[i - 1] = node.Keys[left_size + i];
            for (int i = 0; i < right_size; ++i)
            {
                newNode.children[i] = node.children[left_size + i + 1];
            }
            new_node_pos = BPTree.write(newNode);
            ++tree_size;
            for (int i = 0; i < newNode.size + 1; ++i)
            {
                Node<order> childNode;
                BPTree.read(childNode, newNode.children[i]);
                childNode.parent = new_node_pos;
                BPTree.update(childNode, newNode.children[i]);
            }
        }
        node.size = left_size;
        // Insert the middle key into the parent node
        sjtu::pair<T, int> midKey = node.Keys[left_size];
        if (node.parent == -1) // node is root, need to create new root
        {
            Node<order> newRoot;
            newRoot.isLeaf = false;
            newRoot.size = 1;
            newRoot.Keys[0] = midKey;
            newRoot.children[0] = node_pos;
            newRoot.children[1] = new_node_pos;
            root_pos = node.parent = BPTree.write(newRoot);
            ++tree_size;
            newNode.parent = node.parent;
        }
        else
        {
            Node<order> parentNode;
            BPTree.read(parentNode, node.parent);
            int pos = BinarySearch(parentNode.Keys, parentNode.size, midKey);
            for (int i = parentNode.size; i > pos; --i)
                parentNode.Keys[i] = parentNode.Keys[i - 1];
            for (int i = parentNode.size + 1; i > pos + 1; --i)
                parentNode.children[i] = parentNode.children[i - 1];
            parentNode.Keys[pos] = midKey;
            parentNode.children[pos + 1] = new_node_pos;
            ++parentNode.size;
            if (parentNode.size > max_keys) // parent node overflow, need to split
            {
                is_parent_split = true;
                split(parentNode, node.parent);
            }
            else
                BPTree.update(parentNode, node.parent);
        }
        BPTree.update(node, node_pos);
        if (!is_parent_split)
            BPTree.update(newNode, new_node_pos);
    }

    sjtu::pair<T, int> subtree_min_key(int node_pos)
    {
        Node<order> node;
        BPTree.read(node, node_pos);
        while (!node.isLeaf)
        {
            node_pos = node.children[0];
            BPTree.read(node, node_pos);
        }
        return node.Keys[0];
    }

    void fix_parent(int node_pos, sjtu::vector<int> trace_index)
    {
        // 提前拿到新的最小值，避免在循环中重复读取
        sjtu::pair<T, int> new_min = subtree_min_key(node_pos);
        Node<order> node;
        BPTree.read(node, node_pos);

        while (node.parent != -1)
        {
            Node<order> parentNode;
            BPTree.read(parentNode, node.parent);
            int child_index = -1;

            if (!trace_index.empty())
            {
                child_index = trace_index.back();
                trace_index.pop_back();
            }

            if (child_index < 0 || child_index > parentNode.size || parentNode.children[child_index] != node_pos)
            {
                for (int i = 0; i <= parentNode.size; ++i)
                {
                    if (parentNode.children[i] == node_pos)
                    {
                        child_index = i;
                        break;
                    }
                }
            }

            if (child_index == 0)
            {
                node_pos = node.parent;
                node = parentNode;
                continue;
            }

            parentNode.Keys[child_index - 1] = new_min;
            BPTree.update(parentNode, node.parent);

            return;
        }
    }
    void merge(Node<order> &node, sjtu::vector<int> &trace_index, int node_pos)
    {
        if (node.parent == -1)
        {
            if (!node.isLeaf && node.size == 0)
            {
                root_pos = node.children[0];
                Node<order> newRoot;
                BPTree.read(newRoot, root_pos);
                newRoot.parent = -1;
                BPTree.update(newRoot, root_pos);
                BPTree.Delete(node_pos);
                --tree_size;
            }
            return;
        }
        Node<order> parentNode;
        int parent_pos = node.parent;
        BPTree.read(parentNode, parent_pos);
        int index = -1;
        if (!trace_index.empty())
        {
            index = trace_index.back();
            trace_index.pop_back();
        }
        if (index < 0 || index > parentNode.size || parentNode.children[index] != node_pos)
        {
            for (int i = 0; i <= parentNode.size; ++i)
            {
                if (parentNode.children[i] == node_pos)
                {
                    index = i;
                    break;
                }
            }
        }
        if (index == -1)
            return;
        bool HasLeftSibling = index > 0;
        bool HasRightSibling = index < parentNode.size;
        if (node.isLeaf)
        {
            if (HasLeftSibling)
            {
                Node<order> leftSibling;
                BPTree.read(leftSibling, parentNode.children[index - 1]);
                if (leftSibling.size > min_leaf_keys_non_root) // borrow from left sibling
                {
                    for (int i = node.size; i > 0; --i)
                        node.Keys[i] = node.Keys[i - 1];
                    node.Keys[0] = leftSibling.Keys[leftSibling.size - 1];
                    parentNode.Keys[index - 1] = node.Keys[0];
                    --leftSibling.size;
                    ++node.size;
                    BPTree.update(parentNode, node.parent);
                    if (index == 1)
                        fix_parent(node.parent, trace_index);
                    BPTree.update(leftSibling, parentNode.children[index - 1]);
                    BPTree.update(node, node_pos);
                    return;
                }
            }
            if (HasRightSibling)
            {
                Node<order> rightSibling;
                BPTree.read(rightSibling, parentNode.children[index + 1]);
                if (rightSibling.size > min_leaf_keys_non_root) // borrow from right sibling
                {
                    node.Keys[node.size] = rightSibling.Keys[0];
                    for (int i = 0; i < rightSibling.size - 1; ++i)
                        rightSibling.Keys[i] = rightSibling.Keys[i + 1];
                    parentNode.Keys[index] = rightSibling.Keys[0];
                    --rightSibling.size;
                    ++node.size;
                    BPTree.update(parentNode, node.parent);
                    if (index == 0)
                        fix_parent(node.parent, trace_index);
                    BPTree.update(rightSibling, parentNode.children[index + 1]);
                    BPTree.update(node, node_pos);
                    return;
                }
            }
            if (HasLeftSibling)
            {
                Node<order> leftSibling;
                BPTree.read(leftSibling, parentNode.children[index - 1]);
                for (int i = leftSibling.size; i < leftSibling.size + node.size; ++i)
                    leftSibling.Keys[i] = node.Keys[i - leftSibling.size];
                leftSibling.size += node.size;
                leftSibling.next = node.next;
                BPTree.update(leftSibling, parentNode.children[index - 1]);
                BPTree.Delete(node_pos);
                tree_size--;
                for (int i = index; i < parentNode.size; ++i)
                    parentNode.Keys[i - 1] = parentNode.Keys[i];
                for (int i = index + 1; i < parentNode.size + 1; ++i)
                    parentNode.children[i - 1] = parentNode.children[i];
                --parentNode.size;
                if (parentNode.size < min_internal_keys_non_root)
                {
                    merge(parentNode, trace_index, parent_pos);
                }
                else
                {
                    BPTree.update(parentNode, parent_pos);
                    if (index == 1)
                        fix_parent(parent_pos, trace_index);
                }
                return;
            }
            if (HasRightSibling)
            {
                Node<order> rightSibling;
                BPTree.read(rightSibling, parentNode.children[index + 1]);
                for (int i = node.size; i < node.size + rightSibling.size; ++i)
                    node.Keys[i] = rightSibling.Keys[i - node.size];
                node.size += rightSibling.size;
                node.next = rightSibling.next;
                BPTree.update(node, node_pos);
                BPTree.Delete(parentNode.children[index + 1]);
                tree_size--;
                for (int i = index + 1; i < parentNode.size; ++i)
                    parentNode.Keys[i - 1] = parentNode.Keys[i];
                for (int i = index + 2; i < parentNode.size + 1; ++i)
                    parentNode.children[i - 1] = parentNode.children[i];
                --parentNode.size;
                if (parentNode.size < min_internal_keys_non_root)
                {
                    merge(parentNode, trace_index, parent_pos);
                }
                else
                {
                    BPTree.update(parentNode, parent_pos);
                    if (index == 0)
                        fix_parent(parent_pos, trace_index);
                }
                return;
            }
        }
        else
        {
            if (HasLeftSibling)
            {
                Node<order> leftSibling;
                BPTree.read(leftSibling, parentNode.children[index - 1]);
                if (leftSibling.size > min_internal_keys_non_root) // borrow from left sibling
                {
                    for (int i = node.size; i > 0; --i)
                        node.Keys[i] = node.Keys[i - 1];
                    for (int i = node.size + 1; i > 0; --i)
                        node.children[i] = node.children[i - 1];
                    node.Keys[0] = parentNode.Keys[index - 1];
                    node.children[0] = leftSibling.children[leftSibling.size];
                    Node<order> moveNode;
                    BPTree.read(moveNode, node.children[0]);
                    moveNode.parent = node_pos;
                    BPTree.update(moveNode, node.children[0]);
                    parentNode.Keys[index - 1] = leftSibling.Keys[leftSibling.size - 1];
                    --leftSibling.size;
                    ++node.size;
                    BPTree.update(parentNode, node.parent);
                    if (index == 1)
                        fix_parent(node.parent, trace_index);
                    BPTree.update(leftSibling, parentNode.children[index - 1]);
                    BPTree.update(node, node_pos);
                    return;
                }
            }
            if (HasRightSibling)
            {
                Node<order> rightSibling;
                BPTree.read(rightSibling, parentNode.children[index + 1]);
                if (rightSibling.size > min_internal_keys_non_root) // borrow from right sibling
                {
                    node.Keys[node.size] = parentNode.Keys[index];
                    node.children[node.size + 1] = rightSibling.children[0];
                    parentNode.Keys[index] = rightSibling.Keys[0];
                    for (int i = 0; i < rightSibling.size - 1; ++i)
                        rightSibling.Keys[i] = rightSibling.Keys[i + 1];
                    for (int i = 0; i < rightSibling.size; ++i)
                        rightSibling.children[i] = rightSibling.children[i + 1];
                    Node<order> moveNode;
                    BPTree.read(moveNode, node.children[node.size + 1]);
                    moveNode.parent = node_pos;
                    BPTree.update(moveNode, node.children[node.size + 1]);
                    --rightSibling.size;
                    ++node.size;
                    BPTree.update(parentNode, node.parent);
                    if (index == 0)
                        fix_parent(node.parent, trace_index);
                    BPTree.update(rightSibling, parentNode.children[index + 1]);
                    BPTree.update(node, node_pos);
                    return;
                }
            }
            if (HasLeftSibling)
            {
                Node<order> leftSibling;
                BPTree.read(leftSibling, parentNode.children[index - 1]);
                int old_left_size = leftSibling.size;
                leftSibling.Keys[old_left_size] = parentNode.Keys[index - 1];
                for (int i = 0; i < node.size; ++i)
                    leftSibling.Keys[old_left_size + 1 + i] = node.Keys[i];
                for (int i = 0; i <= node.size; ++i)
                {
                    leftSibling.children[old_left_size + 1 + i] = node.children[i];
                    Node<order> moveNode;
                    BPTree.read(moveNode, node.children[i]);
                    moveNode.parent = parentNode.children[index - 1];
                    BPTree.update(moveNode, node.children[i]);
                }
                leftSibling.size = old_left_size + node.size + 1;
                BPTree.update(leftSibling, parentNode.children[index - 1]);
                BPTree.Delete(node_pos);
                tree_size--;
                for (int i = index; i < parentNode.size; ++i)
                    parentNode.Keys[i - 1] = parentNode.Keys[i];
                for (int i = index + 1; i < parentNode.size + 1; ++i)
                    parentNode.children[i - 1] = parentNode.children[i];
                --parentNode.size;
                if (parentNode.size < min_internal_keys_non_root)
                {
                    merge(parentNode, trace_index, parent_pos);
                }
                else
                {
                    BPTree.update(parentNode, parent_pos);
                    if (index == 1)
                        fix_parent(parent_pos, trace_index);
                }
                return;
            }
            if (HasRightSibling)
            {
                Node<order> rightSibling;
                BPTree.read(rightSibling, parentNode.children[index + 1]);
                int old_node_size = node.size;
                node.Keys[old_node_size] = parentNode.Keys[index];
                for (int i = 0; i < rightSibling.size; ++i)
                    node.Keys[old_node_size + 1 + i] = rightSibling.Keys[i];
                for (int i = 0; i <= rightSibling.size; ++i)
                {
                    node.children[old_node_size + 1 + i] = rightSibling.children[i];
                    Node<order> moveNode;
                    BPTree.read(moveNode, rightSibling.children[i]);
                    moveNode.parent = node_pos;
                    BPTree.update(moveNode, rightSibling.children[i]);
                }
                node.size = old_node_size + rightSibling.size + 1;
                BPTree.update(node, node_pos);
                BPTree.Delete(parentNode.children[index + 1]);
                tree_size--;
                for (int i = index + 1; i < parentNode.size; ++i)
                    parentNode.Keys[i - 1] = parentNode.Keys[i];
                for (int i = index + 2; i < parentNode.size + 1; ++i)
                    parentNode.children[i - 1] = parentNode.children[i];
                --parentNode.size;
                if (parentNode.size < min_internal_keys_non_root)
                {
                    merge(parentNode, trace_index, parent_pos);
                }
                else
                {
                    BPTree.update(parentNode, parent_pos);
                    if (index == 0)
                        fix_parent(parent_pos, trace_index);
                }
                return;
            }
        }
    }

public:
    BPT()
    {
        BPTree.initialise("BPTree.dat");
        BPTree.get_info(root_pos, 1);
        BPTree.get_info(tree_size, 2);
    }
    ~BPT()
    {
        BPTree.write_info(root_pos, 1);
        BPTree.write_info(tree_size, 2);
    }
    void insert(const T &key, int value)
    {
        sjtu::pair<T, int> keyValuePair(key, value);
        if (tree_size == 0) // tree is empty, create root node
        {
            Node<order> root;
            root.isLeaf = true;
            root.size = 1;
            root.Keys[0] = keyValuePair;
            root.parent = -1;
            root.next = -1;
            root_pos = BPTree.write(root);
            tree_size = 1;
        }
        else
        {
            Node<order> node;
            BPTree.read(node, root_pos);
            int node_pos = root_pos;
            while (!node.isLeaf)
            {
                // Keep order by (key, value) so duplicated keys with different values are all indexed.
                int child_index = BinarySearch(node.Keys, node.size, keyValuePair);
                node_pos = node.children[child_index];
                BPTree.read(node, node_pos);
            }
            int insert_index = BinarySearch(node.Keys, node.size, keyValuePair);
            for (int i = node.size; i > insert_index; --i)
                node.Keys[i] = node.Keys[i - 1];
            node.Keys[insert_index] = keyValuePair;
            ++node.size;
            if (node.size > max_keys) // node overflow, need to split
                split(node, node_pos);
            else
                BPTree.update(node, node_pos);
        }
    }
    void remove(const T &key, int value)
    {
        if (tree_size == 0)
        {
            return;
        }
        sjtu::vector<int> trace_index; // index of the child in the parent node
        Node<order> node;
        BPTree.read(node, root_pos);
        int node_pos = root_pos;
        sjtu::pair<T, int> keyValuePair(key, value);
        while (!node.isLeaf)
        {
            int child_index = BinarySearch(node.Keys, node.size, keyValuePair);
            trace_index.push_back(child_index);
            node_pos = node.children[child_index];
            BPTree.read(node, node_pos);
        }
        int upper_index = BinarySearch(node.Keys, node.size, keyValuePair);
        if (upper_index == 0 || node.Keys[upper_index - 1] != keyValuePair)
        {
            return;
        }
        for (int i = upper_index; i < node.size; ++i)
            node.Keys[i - 1] = node.Keys[i];
        --node.size;

        // Persist leaf mutation first so subtree_min_key sees fresh data.
        BPTree.update(node, node_pos);

        // If the deleted key was the first key, subtree minimum may have changed.
        if (upper_index == 1)
            fix_parent(node_pos, trace_index);

        int min_size = min_leaf_keys_non_root; // minimum number of keys in a non-root leaf
        if (node.size >= min_size || node.parent == -1) // node has enough keys or is root, no need to merge
        {
            return;
        }
        // Node underflow, need to merge with sibling
        merge(node, trace_index, node_pos);
    }
    void search(const T &key)
    {
        if (tree_size == 0)
        {
            std::cout << "null\n";
            return;
        }
        Node<order> node;
        BPTree.read(node, root_pos);
        int child_index = BinarySearch(node.Keys, node.size, key);
        while (!node.isLeaf)
        {
            BPTree.read(node, node.children[child_index]);
            child_index = BinarySearch(node.Keys, node.size, key);
        }
        int scan_index = child_index;
        if (scan_index < node.size)
        {
            if (node.Keys[scan_index].first != key)
            {
                std::cout << "null\n";
                return;
            }
        }
        else
        {
            if (node.next == -1)
            {
                std::cout << "null\n";
                return;
            }
            BPTree.read(node, node.next);
            scan_index = 0;
            if (node.Keys[scan_index].first != key)
            {
                std::cout << "null\n";
                return;
            }
        }
        sjtu::pair<T, int> keyValuePair(key, node.Keys[scan_index].second);
        while (keyValuePair.first == key)
        {
            std::cout << keyValuePair.second << ' ';
            if (++scan_index < node.size)
                keyValuePair = node.Keys[scan_index];
            else
            {
                if (node.next == -1)
                {
                    std::cout << '\n';
                    return;
                }
                BPTree.read(node, node.next);
                scan_index = 0;
                keyValuePair = node.Keys[scan_index];
            }
        }
        std::cout << '\n';
    }
};
#endif // BPT.hpp
