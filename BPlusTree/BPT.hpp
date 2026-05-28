#ifndef BPT_HPP
#define BPT_HPP
#include <iostream>
#include "BPT_MemoryRiver.hpp"
#include "BufferPoolManager.hpp"
#include "utility.hpp"
#include "vector.hpp"
constexpr int order = 48; // B+树的阶数，决定了每个节点的最大子节点数和最大键数
template<class T>
inline int BinarySearch(const T arr[], int size, const T &key) // upper_bound
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
inline int BinarySearch(const sjtu::pair<T, int> arr[], int size, const T &key) // lower_bound
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
    MemoryRiver<Node<order>> BPTree;
    BufferPoolManager<Node<order>> *bufferPool;

    using AccessType = typename BufferPoolManager<Node<order>>::AccessType;
    static constexpr AccessType SCAN_TYPE = AccessType::Scan;
    static constexpr AccessType LOOKUP_TYPE = AccessType::Lookup;
    static constexpr AccessType INDEX_TYPE = AccessType::Index;


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
            new_node_pos = bufferPool->new_page(newNode);
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
            new_node_pos = bufferPool->new_page(newNode);
            ++tree_size;
            for (int i = 0; i < newNode.size + 1; ++i)
            {
                Node<order> &childNode = bufferPool->get(newNode.children[i], INDEX_TYPE);
                childNode.parent = new_node_pos;
                bufferPool->put(newNode.children[i], childNode, INDEX_TYPE);
            }
        }
        // Capture middle key before shrinking the left node
        sjtu::pair<T, int> midKey = node.Keys[left_size];
        node.size = left_size;
        // Persist left node now so recursive splits won't be overwritten
        bufferPool->put(node_pos, node, SCAN_TYPE);
        if (node.parent == -1) // node is root, need to create new root
        {
            Node<order> newRoot;
            newRoot.isLeaf = false;
            newRoot.size = 1;
            newRoot.Keys[0] = midKey;
            newRoot.children[0] = node_pos;
            newRoot.children[1] = new_node_pos;
            root_pos = bufferPool->new_page(newRoot);
            ++tree_size;
            node.parent = root_pos;
            newNode.parent = root_pos;
            // write updated parent pointers for the two children
            bufferPool->put(node_pos, node, LOOKUP_TYPE);
            bufferPool->put(new_node_pos, newNode, LOOKUP_TYPE);
        }
        else
        {
            Node<order> &parentNode = bufferPool->get(node.parent, INDEX_TYPE);
            int pos = BinarySearch(parentNode.Keys, parentNode.size, midKey);
            for (int i = parentNode.size; i > pos; --i)
                parentNode.Keys[i] = parentNode.Keys[i - 1];
            for (int i = parentNode.size + 1; i > pos + 1; --i)
                parentNode.children[i] = parentNode.children[i - 1];
            parentNode.Keys[pos] = midKey;
            parentNode.children[pos + 1] = new_node_pos;
            ++parentNode.size;
            // parentNode updated: if it overflows, split it (recursive).
            // Do NOT re-write child nodes here — they were persisted above.
            if (parentNode.size > max_keys) // parent node overflow, need to split
            {
                split(parentNode, node.parent);
            }
            else
                bufferPool->put(node.parent, parentNode, INDEX_TYPE);
        }
    }

    void fix_parent(int node_pos, const sjtu::pair<T, int> &new_min, sjtu::vector<int> &trace_index)
    {
        Node<order> node = bufferPool->get(node_pos, SCAN_TYPE);

        while (node.parent != -1)
        {
            Node<order> parentNode = bufferPool->get(node.parent, INDEX_TYPE);
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
            bufferPool->put(node.parent, parentNode, INDEX_TYPE);

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
                Node<order> newRoot = bufferPool->get(root_pos, LOOKUP_TYPE);
                newRoot.parent = -1;
                bufferPool->put(root_pos, newRoot, LOOKUP_TYPE);
                bufferPool->Delete(node_pos);
                --tree_size;
            }
            else
            {
                bufferPool->put(node_pos, node, LOOKUP_TYPE);
            }
            return;
        }
        Node<order> parentNode = bufferPool->get(node.parent, INDEX_TYPE);
        int parent_pos = node.parent;
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
                Node<order> leftSibling = bufferPool->get(parentNode.children[index - 1], SCAN_TYPE);
                if (leftSibling.size > min_leaf_keys_non_root) // borrow from left sibling
                {
                    for (int i = node.size; i > 0; --i)
                        node.Keys[i] = node.Keys[i - 1];
                    node.Keys[0] = leftSibling.Keys[leftSibling.size - 1];
                    parentNode.Keys[index - 1] = node.Keys[0];
                    --leftSibling.size;
                    ++node.size;
                    bufferPool->put(node.parent, parentNode, INDEX_TYPE);
                    bufferPool->put(parentNode.children[index - 1], leftSibling, SCAN_TYPE);
                    bufferPool->put(node_pos, node, SCAN_TYPE);
                    return;
                }
            }
            if (HasRightSibling)
            {
                Node<order> rightSibling = bufferPool->get(parentNode.children[index + 1], SCAN_TYPE);
                if (rightSibling.size > min_leaf_keys_non_root) // borrow from right sibling
                {
                    node.Keys[node.size] = rightSibling.Keys[0];
                    for (int i = 0; i < rightSibling.size - 1; ++i)
                        rightSibling.Keys[i] = rightSibling.Keys[i + 1];
                    parentNode.Keys[index] = rightSibling.Keys[0];
                    --rightSibling.size;
                    ++node.size;
                    bufferPool->put(node.parent, parentNode, INDEX_TYPE);
                    bufferPool->put(parentNode.children[index + 1], rightSibling, SCAN_TYPE);
                    bufferPool->put(node_pos, node, SCAN_TYPE);
                    return;
                }
            }
            if (HasLeftSibling)
            {
                Node<order> leftSibling = bufferPool->get(parentNode.children[index - 1], SCAN_TYPE);
                for (int i = leftSibling.size; i < leftSibling.size + node.size; ++i)
                    leftSibling.Keys[i] = node.Keys[i - leftSibling.size];
                leftSibling.size += node.size;
                leftSibling.next = node.next;
                bufferPool->put(parentNode.children[index - 1], leftSibling, SCAN_TYPE);
                bufferPool->Delete(node_pos);
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
                    bufferPool->put(parent_pos, parentNode, INDEX_TYPE);
                }
                return;
            }
            if (HasRightSibling)
            {
                Node<order> rightSibling = bufferPool->get(parentNode.children[index + 1], SCAN_TYPE);
                for (int i = node.size; i < node.size + rightSibling.size; ++i)
                    node.Keys[i] = rightSibling.Keys[i - node.size];
                node.size += rightSibling.size;
                node.next = rightSibling.next;
                bufferPool->put(node_pos, node, SCAN_TYPE);
                bufferPool->Delete(parentNode.children[index + 1]);
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
                    bufferPool->put(parent_pos, parentNode, INDEX_TYPE);
                }
                return;
            }
        }
        else
        {
            if (HasLeftSibling)
            {
                Node<order> leftSibling = bufferPool->get(parentNode.children[index - 1], INDEX_TYPE);
                if (leftSibling.size > min_internal_keys_non_root) // borrow from left sibling
                {
                    for (int i = node.size; i > 0; --i)
                        node.Keys[i] = node.Keys[i - 1];
                    for (int i = node.size + 1; i > 0; --i)
                        node.children[i] = node.children[i - 1];
                    node.Keys[0] = parentNode.Keys[index - 1];
                    node.children[0] = leftSibling.children[leftSibling.size];
                    Node<order> moveNode = bufferPool->get(node.children[0], INDEX_TYPE);
                    moveNode.parent = node_pos;
                    bufferPool->put(node.children[0], moveNode, INDEX_TYPE);
                    parentNode.Keys[index - 1] = leftSibling.Keys[leftSibling.size - 1];
                    --leftSibling.size;
                    ++node.size;
                    bufferPool->put(node.parent, parentNode, INDEX_TYPE);
                    bufferPool->put(parentNode.children[index - 1], leftSibling, INDEX_TYPE);
                    bufferPool->put(node_pos, node, INDEX_TYPE);
                    return;
                }
            }
            if (HasRightSibling)
            {
                Node<order> rightSibling = bufferPool->get(parentNode.children[index + 1], INDEX_TYPE);
                if (rightSibling.size > min_internal_keys_non_root) // borrow from right sibling
                {
                    node.Keys[node.size] = parentNode.Keys[index];
                    node.children[node.size + 1] = rightSibling.children[0];
                    parentNode.Keys[index] = rightSibling.Keys[0];
                    for (int i = 0; i < rightSibling.size - 1; ++i)
                        rightSibling.Keys[i] = rightSibling.Keys[i + 1];
                    for (int i = 0; i < rightSibling.size; ++i)
                        rightSibling.children[i] = rightSibling.children[i + 1];
                    Node<order> moveNode = bufferPool->get(node.children[node.size + 1], INDEX_TYPE);
                    moveNode.parent = node_pos;
                    bufferPool->put(node.children[node.size + 1], moveNode, INDEX_TYPE);
                    --rightSibling.size;
                    ++node.size;
                    bufferPool->put(node.parent, parentNode, INDEX_TYPE);
                    bufferPool->put(parentNode.children[index + 1], rightSibling, INDEX_TYPE);
                    bufferPool->put(node_pos, node, INDEX_TYPE);
                    return;
                }
            }
            if (HasLeftSibling)
            {
                Node<order> leftSibling = bufferPool->get(parentNode.children[index - 1], INDEX_TYPE);
                int old_left_size = leftSibling.size;
                leftSibling.Keys[old_left_size] = parentNode.Keys[index - 1];
                for (int i = 0; i < node.size; ++i)
                    leftSibling.Keys[old_left_size + 1 + i] = node.Keys[i];
                for (int i = 0; i <= node.size; ++i)
                {
                    leftSibling.children[old_left_size + 1 + i] = node.children[i];
                    Node<order> moveNode = bufferPool->get(node.children[i], INDEX_TYPE);
                    moveNode.parent = parentNode.children[index - 1];
                    bufferPool->put(node.children[i], moveNode, INDEX_TYPE);
                }
                leftSibling.size = old_left_size + node.size + 1;
                bufferPool->put(parentNode.children[index - 1], leftSibling, INDEX_TYPE);
                bufferPool->Delete(node_pos);
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
                    bufferPool->put(parent_pos, parentNode, INDEX_TYPE);
                }
                return;
            }
            if (HasRightSibling)
            {
                Node<order> rightSibling = bufferPool->get(parentNode.children[index + 1], INDEX_TYPE);
                int old_node_size = node.size;
                node.Keys[old_node_size] = parentNode.Keys[index];
                for (int i = 0; i < rightSibling.size; ++i)
                    node.Keys[old_node_size + 1 + i] = rightSibling.Keys[i];
                for (int i = 0; i <= rightSibling.size; ++i)
                {
                    node.children[old_node_size + 1 + i] = rightSibling.children[i];
                    Node<order> moveNode = bufferPool->get(rightSibling.children[i], INDEX_TYPE);
                    moveNode.parent = node_pos;
                    bufferPool->put(rightSibling.children[i], moveNode, INDEX_TYPE);
                }
                node.size = old_node_size + rightSibling.size + 1;
                bufferPool->put(node_pos, node, INDEX_TYPE);
                bufferPool->Delete(parentNode.children[index + 1]);
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
                    bufferPool->put(parent_pos, parentNode, INDEX_TYPE);
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

        bufferPool = new BufferPoolManager<Node<order>>(6500, BPTree, root_pos);
    }
    ~BPT()
    {
        BPTree.write_info(root_pos, 1);
        BPTree.write_info(tree_size, 2);
        delete bufferPool;
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
            root_pos = bufferPool->new_page(root);
            tree_size = 1;
        }
        else
        {
            sjtu::vector<int> trace_index; // index of the child in the parent node
            Node<order> node = bufferPool->get(root_pos, LOOKUP_TYPE);
            int node_pos = root_pos;
            while (!node.isLeaf)
            {
                // Keep order by (key, value) so duplicated keys with different values are all indexed.
                int child_index = BinarySearch(node.Keys, node.size, keyValuePair);
                trace_index.push_back(child_index);
                node_pos = node.children[child_index];
                node = bufferPool->get(node_pos, INDEX_TYPE);
            }
            int insert_index = BinarySearch(node.Keys, node.size, keyValuePair);
            if (node.size < max_keys) // node has space, insert directly
            {
                for (int i = node.size; i > insert_index; --i)
                    node.Keys[i] = node.Keys[i - 1];
                node.Keys[insert_index] = keyValuePair;
                ++node.size;
                bufferPool->put(node_pos, node, SCAN_TYPE);
                return;
            }
            else
            {
                if (trace_index.empty())
                {
                    for (int i = node.size; i > insert_index; --i)
                        node.Keys[i] = node.Keys[i - 1];
                    node.Keys[insert_index] = keyValuePair;
                    ++node.size;
                    split(node, node_pos);
                }
                else
                {
                    auto index = trace_index.back();
                    auto &parentNode = bufferPool->get(node.parent, INDEX_TYPE);
                    bool HasLeftSibling = index > 0;
                    bool HasRightSibling = index < parentNode.size;
                    if (HasLeftSibling)
                    {
                        auto &leftSibling = bufferPool->get(parentNode.children[index - 1], SCAN_TYPE);
                        if (insert_index > 0 && leftSibling.size + insert_index <= max_keys) // insert into left sibling
                        {
                            for (int i = leftSibling.size; i < insert_index + leftSibling.size; ++i)
                                leftSibling.Keys[i] = node.Keys[i - leftSibling.size];
                            for (int i = 1; i < node.size - insert_index + 1; ++i)
                                node.Keys[i] = node.Keys[insert_index + i - 1];
                            node.Keys[0] = keyValuePair;
                            leftSibling.size += insert_index;
                            node.size -= insert_index - 1;
                            fix_parent(node_pos, node.Keys[0], trace_index);
                            bufferPool->put(parentNode.children[index - 1], leftSibling, SCAN_TYPE);
                            bufferPool->put(node_pos, node, SCAN_TYPE);
                            return;
                        }
                    }
                    if (HasRightSibling)
                    {
                        auto &rightSibling = bufferPool->get(parentNode.children[index + 1], SCAN_TYPE);

                        // Build a temporary sorted array after inserting keyValuePair.
                        sjtu::pair<T, int> temp[max_keys + 1];
                        for (int i = 0; i < insert_index; ++i)
                            temp[i] = node.Keys[i];
                        temp[insert_index] = keyValuePair;
                        for (int i = insert_index; i < node.size; ++i)
                            temp[i + 1] = node.Keys[i];

                        // Keep the left part in current node, push the suffix to right sibling.
                        int keep_in_node = insert_index + 1;
                        if (keep_in_node > max_keys)
                            keep_in_node = max_keys;
                        int move_count = node.size + 1 - keep_in_node;

                        if (rightSibling.size + move_count <= max_keys)
                        {
                            for (int i = rightSibling.size - 1; i >= 0; --i)
                                rightSibling.Keys[i + move_count] = rightSibling.Keys[i];
                            for (int i = 0; i < move_count; ++i)
                                rightSibling.Keys[i] = temp[keep_in_node + i];
                            for (int i = 0; i < keep_in_node; ++i)
                                node.Keys[i] = temp[i];

                            node.size = keep_in_node;
                            rightSibling.size += move_count;

                            bufferPool->put(parentNode.children[index + 1], rightSibling, SCAN_TYPE);
                            bufferPool->put(node_pos, node, SCAN_TYPE);

                            // Node minimum key may change if inserted at position 0.
                            if (insert_index == 0)
                                fix_parent(node_pos, node.Keys[0], trace_index);

                            // Separator between current node and right sibling must always be refreshed.
                            Node<order> parentRefreshed = bufferPool->get(node.parent, INDEX_TYPE);
                            parentRefreshed.Keys[index] = rightSibling.Keys[0];
                            bufferPool->put(node.parent, parentRefreshed, INDEX_TYPE);
                            return;
                        }
                    }
                    for (int i = node.size; i > insert_index; --i)
                        node.Keys[i] = node.Keys[i - 1];
                    node.Keys[insert_index] = keyValuePair;
                    ++node.size;
                    split(node, node_pos);
                }
            }
        }
    }
    void remove(const T &key, int value)
    {
        if (tree_size == 0)
        {
            return;
        }
        sjtu::vector<int> trace_index; // index of the child in the parent node
        Node<order> node = bufferPool->get(root_pos, LOOKUP_TYPE);
        int node_pos = root_pos;
        sjtu::pair<T, int> keyValuePair(key, value);
        while (!node.isLeaf)
        {
            int child_index = BinarySearch(node.Keys, node.size, keyValuePair);
            trace_index.push_back(child_index);
            node_pos = node.children[child_index];
            node = bufferPool->get(node_pos, INDEX_TYPE);
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
        bufferPool->put(node_pos, node, SCAN_TYPE);

        // If the deleted key was the first key, subtree minimum may have changed.
        if (upper_index == 1)
            fix_parent(node_pos, node.Keys[0], trace_index);

        int min_size = min_leaf_keys_non_root; // minimum number of keys in a non-root leaf
        if (node.size >= min_size || node.parent == -1) // node has enough keys or is root, no need to merge
        {
            return;
        }
        // Node underflow, need to merge with sibling
        merge(node, trace_index, node_pos);
    }
    inline char *fast_itoa(int val, char *buf)
    {
        if (val == 0)
        {
            *buf++ = '0';
            return buf;
        }
        // 处理负数（如果 int 范围内包含负数）
        if (val < 0)
        {
            *buf++ = '-';
            val = -val;
        }

        char temp[12]; // int 最大 10 位 + 符号
        int i = 0;
        while (val > 0)
        {
            temp[i++] = (val % 10) + '0';
            val /= 10;
        }
        // 反向写入 buffer
        while (i > 0)
        {
            *buf++ = temp[--i];
        }
        return buf;
    }

    void search(const T &key)
    {
        if (tree_size == 0)
        {
            std::cout << "null\n";
            return;
        }

        Node<order> node = bufferPool->get(root_pos, LOOKUP_TYPE);
        int child_index = BinarySearch(node.Keys, node.size, key);
        while (!node.isLeaf)
        {
            node = bufferPool->get(node.children[child_index], INDEX_TYPE);
            child_index = BinarySearch(node.Keys, node.size, key);
        }

        int scan_index = child_index;
        if (scan_index >= node.size)
        {
            if (node.next == -1)
            {
                std::cout << "null\n";
                return;
            }
            node = bufferPool->get(node.next, INDEX_TYPE);
            scan_index = 0;
        }

        if (node.Keys[scan_index].first != key)
        {
            std::cout << "null\n";
            return;
        }

        static char out_buf[131072];
        char *ptr = out_buf;
        const char *const buf_end = out_buf + 131000;
        bool found_any = false;

        // 模拟 keyValuePair 比较
        while (node.Keys[scan_index].first == key)
        {
            found_any = true;

            // 如果缓冲区接近溢出，先刷新到 stdout
            if (ptr >= buf_end)
            {
                std::cout.write(out_buf, ptr - out_buf);
                ptr = out_buf;
            }

            // 直接将整数格式化进缓冲区，不产生任何临时 string 对象
            ptr = fast_itoa(node.Keys[scan_index].second, ptr);
            *ptr++ = ' ';

            // 迭代到下一个 Key
            if (++scan_index >= node.size)
            {
                if (node.next == -1)
                    break;
                node = bufferPool->get(node.next, SCAN_TYPE);
                scan_index = 0;
            }
        }

        if (!found_any)
        {
            std::cout << "null\n";
        }
        else
        {
            *ptr++ = '\n';
            std::cout.write(out_buf, ptr - out_buf);
        }
    }
};
#endif // BPT.hpp
