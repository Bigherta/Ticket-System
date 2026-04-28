#ifndef BUFFERPOOLMANAGER_HPP
#define BUFFERPOOLMANAGER_HPP
#include "BPT_MemoryRiver.hpp"
#include "list.hpp"
#include "unordered_map.hpp"
template<class Page>
class BufferPoolManager
{
private:
    struct Frame
    {
        int page_id;
        Page page;
        bool dirty = false;
        bool is_deleted = false;
    };

    int &root_pos;

    int capacity;

    // LRU：head : most recently used, tail : least recently used
    sjtu::list<Frame> lru;

    // page_id -> iterator in lru
    sjtu::unordered_map<int, typename sjtu::list<Frame>::iterator> page_table;

    MemoryRiver<Page> &disk;

public:
    BufferPoolManager(int cap, MemoryRiver<Page> &mr, int &root) : capacity(cap), disk(mr), root_pos(root) {}

    Page &get(int page_id)
    {
        auto it = page_table.find(page_id);

        if (it != page_table.end())
        {
            // cache hit
            lru.splice(lru.begin(), lru, it->second);
            if (it->second->is_deleted)
                throw std::runtime_error("access deleted page");
            return it->second->page;
        }

        // cache miss, read from disk
        Page page;
        disk.read(page, page_id);

        // if cache is full, evict the least recently used page
        if (lru.size() == capacity)
            evict();

        lru.push_front({page_id, page, false, false});
        page_table[page_id] = lru.begin();

        return lru.begin()->page;
    }
    void put(int page_id, const Page &page)
    {
        auto it = page_table.find(page_id);

        if (it != page_table.end())
        {
            // update existing page
            it->second->page = page;
            it->second->dirty = true;
            it->second->is_deleted = false;
            lru.splice(lru.begin(), lru, it->second);
        }
        else
        {
            // insert new page
            if (lru.size() == capacity)
                evict();

            lru.push_front({page_id, page, true, false});
            page_table[page_id] = lru.begin();
        }
    }

    void Delete(int page_id)
    {
        auto it = page_table.find(page_id);

        // remove from buffer pool if exists
        if (it != page_table.end())
        {
            lru.erase(it->second);
            page_table.erase(it);
        }

        // mark as deleted in disk
        disk.Delete(page_id);
    }

    int new_page(Page &page)
    {
        int pos = disk.write(page);
        if (lru.size() == capacity)
            evict();
        lru.push_front({pos, page, false, false});
        page_table[pos] = lru.begin();
        return pos;
    }

    void evict()
    {
        auto it = lru.end();

        while (it != lru.begin())
        {
            --it;

            if (it->page_id == root_pos)
                continue;

            if (it->dirty && !it->is_deleted)
                disk.update(it->page, it->page_id);

            if (it->is_deleted)
                disk.Delete(it->page_id);

            page_table.erase(it->page_id);
            lru.erase(it);
            return;
        }

        throw std::runtime_error("no evictable page");
    }

    void flush_all()
    {
        for (auto it = lru.begin(); it != lru.end();)
        {
            if (it->is_deleted)
            {
                disk.Delete(it->page_id);
                page_table.erase(it->page_id);
                it = lru.erase(it); 
            }
            else
            {
                if (it->dirty)
                {
                    disk.update(it->page, it->page_id);
                    it->dirty = false;
                }
                ++it;
            }
        }
    }
    ~BufferPoolManager() { flush_all(); }
};
#endif // BUFFERPOOLMANAGER_HPP
