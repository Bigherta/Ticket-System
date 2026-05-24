#ifndef BPT_MEMORYRIVER_HPP
#define BPT_MEMORYRIVER_HPP

#include <fstream>

using std::fstream;
using std::ifstream;
using std::ofstream;
using std::string;

template<class T, int info_len = 3>
class MemoryRiver
{
private:
    fstream file;
    string file_name;
    int sizeofT = sizeof(T);

    static constexpr int FREE_LIST_SLOT = info_len - 1; // zero-based index

    void ensure_data_file_open()
    {
        if (!file.is_open())
            file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
    }

public:
    MemoryRiver() = default;

    MemoryRiver(const string &file_name) : file_name(file_name) {}

    ~MemoryRiver()
    {
        if (file.is_open())
            file.close();
    }

    void initialise(string FN = "")
    {
        if (!FN.empty())
            file_name = FN;

        // Open data file. If it does not exist, create it and initialize
        // the info area (all zeros).
        file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
        if (!file.is_open())
        {
            file.open(file_name, std::ios::out | std::ios::binary);
            int tmp = 0;
            for (int i = 0; i < info_len; ++i)
                file.write(reinterpret_cast<char *>(&tmp), sizeof(int));
            file.close();
            file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
        }
    }

    // Read the n-th int from the info area (1-based).
    void get_info(int &tmp, int n)
    {
        if (n < 1 || n > info_len)
            return;
        ensure_data_file_open();
        file.clear();
        file.seekg((n - 1) * sizeof(int));
        file.read(reinterpret_cast<char *>(&tmp), sizeof(int));
    }

    // Write the n-th int into the info area (1-based).
    void write_info(int tmp, int n)
    {
        if (n < 1 || n > info_len)
            return;
        ensure_data_file_open();
        file.clear();
        file.seekp((n - 1) * sizeof(int));
        file.write(reinterpret_cast<char *>(&tmp), sizeof(int));
    }

    // Allocate space for a new object T and write it. Returns the
    // byte-offset where the object was written.
    // If the free list is non-empty, reuse a previously deleted block.
    int write(T &t)
    {
        ensure_data_file_open();
        file.clear();

        // Read current free-list head.
        int free_head = 0;
        get_info(free_head, FREE_LIST_SLOT + 1); // 1-based

        int index;
        if (free_head != 0)
        {
            // Reuse the freed block. First, read the "next" pointer stored
            // at the beginning of the freed block to update the head.
            int next_free = 0;
            file.seekg(free_head);
            file.read(reinterpret_cast<char *>(&next_free), sizeof(int));

            // Overwrite the freed block with the new object.
            file.seekp(free_head);
            file.write(reinterpret_cast<char *>(&t), sizeofT);

            // Update free-list head to next_free.
            write_info(next_free, FREE_LIST_SLOT + 1);

            index = free_head;
        }
        else
        {
            // Free list is empty; append to end of file.
            file.seekp(0, std::ios::end);
            index = file.tellp();
            file.write(reinterpret_cast<char *>(&t), sizeofT);
        }
        return index;
    }

    // Overwrite the object at byte-offset 'index'.
    void update(T &t, const int index)
    {
        ensure_data_file_open();
        file.clear();
        file.seekp(index);
        file.write(reinterpret_cast<char *>(&t), sizeofT);
    }

    // Read the object at byte-offset 'index' into t.
    void read(T &t, const int index)
    {
        ensure_data_file_open();
        file.clear();
        file.seekg(index);
        file.read(reinterpret_cast<char *>(&t), sizeofT);
    }

    // Free the block at byte-offset 'index'.
    // Pushes 'index' onto the embedded free list.
    void Delete(int index)
    {
        ensure_data_file_open();
        file.clear();

        // Read current free-list head.
        int free_head = 0;
        get_info(free_head, FREE_LIST_SLOT + 1); // 1-based

        // Write the current head as the "next" pointer at the beginning
        // of the freed block.
        file.seekp(index);
        file.write(reinterpret_cast<char *>(&free_head), sizeof(int));

        // Update free-list head to point to this newly freed block.
        write_info(index, FREE_LIST_SLOT + 1);
    }

    void clear()
    {
        if (file.is_open())
            file.close();
        file.open(file_name, std::ios::out | std::ios::trunc | std::ios::binary);
        int tmp = 0;
        for (int i = 0; i < info_len; ++i)
            file.write(reinterpret_cast<char *>(&tmp), sizeof(int));
        file.close();
    }
};

#endif // BPT_MEMORYRIVER_HPP