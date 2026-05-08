#ifndef BUFFERPOOLMANAGER_HPP
#define BUFFERPOOLMANAGER_HPP
#include "BPT_MemoryRiver.hpp"
#include "list.hpp"
#include "unordered_map.hpp"

template<class Page>
class BufferPoolManager
{
private:
    enum ArcStatus
    {
        T1, // Recent cache list (mru)
        T2, // Frequent cache list (mfu)
        B1, // Ghost list from T1
        B2 // Ghost list from T2
    };

    struct Frame
    {
        int page_id;
        Page page;
        bool dirty = false;
        bool is_deleted = false;
        ArcStatus status = T1;
    };

    struct GhostEntry
    {
        int page_id;
    };

    int &root_pos;
    int capacity;

    // T1: recent cache
    sjtu::list<Frame> t1;
    // T2: frequent cache
    sjtu::list<Frame> t2;
    // B1: ghost entries from T1
    sjtu::list<GhostEntry> b1;
    // B2: ghost entries from T2
    sjtu::list<GhostEntry> b2;

    struct PageTableEntry
    {
        typename sjtu::list<Frame>::iterator frame_it;
        ArcStatus status;
    };

    struct GhostPageTableEntry
    {
        typename sjtu::list<GhostEntry>::iterator ghost_it;
        ArcStatus status;
    };

    sjtu::unordered_map<int, PageTableEntry> page_table;
    sjtu::unordered_map<int, GhostPageTableEntry> ghost_table;

    int p = 0;

    MemoryRiver<Page> &disk;

    int get_t1_size() const { return t1.size(); }
    int get_t2_size() const { return t2.size(); }
    int get_b1_size() const { return b1.size(); }
    int get_b2_size() const { return b2.size(); }

    void replace(bool hit_in_ghost_b2 = false)
    {
        bool evict_from_t1 = false;

        if (get_t1_size() > 0 && (get_t1_size() > p || (hit_in_ghost_b2 && get_t1_size() == p)))
        {
            evict_from_t1 = true;
        }
        else if (get_t2_size() > 0)
        {
            evict_from_t1 = false;
        }
        else if (get_t1_size() > 0)
        {
            evict_from_t1 = true;
        }

        if (evict_from_t1)
        {
            evict_from_list(t1, B1, b1);
        }
        else
        {
            evict_from_list(t2, B2, b2);
        }
    }

    void evict_from_list(sjtu::list<Frame> &list, ArcStatus ghost_status, sjtu::list<GhostEntry> &ghost_list)
    {
        auto it = list.end();

        while (it != list.begin())
        {
            --it;

            if (it->page_id == root_pos)
                continue;

            int pid = it->page_id;

            if (it->dirty && !it->is_deleted)
                disk.update(it->page, pid);

            if (it->is_deleted)
                disk.Delete(pid);

            if (!it->is_deleted)
            {
                ghost_list.push_front({pid});
                ghost_table[pid] = {ghost_list.begin(), ghost_status};

                if (get_b1_size() + get_b2_size() > capacity)
                {
                    if (ghost_status == B1 && get_b1_size() > capacity)
                    {
                        auto ghost_it = b1.end();
                        --ghost_it;
                        ghost_table.erase(ghost_it->page_id);
                        b1.erase(ghost_it);
                    }
                    else if (ghost_status == B2 && get_b2_size() > capacity * 2)
                    {
                        auto ghost_it = b2.end();
                        --ghost_it;
                        ghost_table.erase(ghost_it->page_id);
                        b2.erase(ghost_it);
                    }
                    else if (get_b1_size() + get_b2_size() > capacity)
                    {
                        if (ghost_status == B1)
                        {
                            auto ghost_it = b2.end();
                            if (ghost_it != b2.begin())
                            {
                                --ghost_it;
                                ghost_table.erase(ghost_it->page_id);
                                b2.erase(ghost_it);
                            }
                        }
                        else
                        {
                            auto ghost_it = b1.end();
                            if (ghost_it != b1.begin())
                            {
                                --ghost_it;
                                ghost_table.erase(ghost_it->page_id);
                                b1.erase(ghost_it);
                            }
                        }
                    }
                }
            }

            page_table.erase(pid);
            list.erase(it);
            return;
        }

        throw std::runtime_error("no evictable page");
    }

public:
    BufferPoolManager(int cap, MemoryRiver<Page> &mr, int &root) : capacity(cap), disk(mr), root_pos(root), p(0) {}

    Page &get(int page_id)
    {
        auto pt_it = page_table.find(page_id);

        if (pt_it != page_table.end())
        {
            auto &entry = pt_it->second;

            if (entry.status == T1)
            {
                auto frame_it = entry.frame_it;
                if (frame_it->is_deleted)
                    throw std::runtime_error("access deleted page");

                Frame frame = *frame_it;
                frame.status = T2;
                t1.erase(frame_it);

                t2.push_front(frame);
                entry.frame_it = t2.begin();
                entry.status = T2;

                return t2.front().page;
            }
            else if (entry.status == T2)
            {
                auto frame_it = entry.frame_it;
                if (frame_it->is_deleted)
                    throw std::runtime_error("access deleted page");

                t2.splice(t2.begin(), t2, frame_it);
                entry.frame_it = t2.begin();

                return t2.front().page;
            }
        }

        auto gt_it = ghost_table.find(page_id);
        bool hit_in_ghost_b2 = false;
        if (gt_it != ghost_table.end())
        {
            auto ghost_entry = gt_it->second;

            if (ghost_entry.status == B1)
            {
                int delta = (get_b1_size() >= get_b2_size() || get_b2_size() == 0) ? 1 : get_b2_size() / get_b1_size();
                p = std::min(capacity, p + delta);
                b1.erase(ghost_entry.ghost_it);
            }
            else if (ghost_entry.status == B2)
            {
                int delta = (get_b2_size() >= get_b1_size() || get_b1_size() == 0) ? 1 : get_b1_size() / get_b2_size();
                p = std::max(0, p - delta);
                b2.erase(ghost_entry.ghost_it);
                hit_in_ghost_b2 = true;
            }

            ghost_table.erase(page_id);

            if (get_t1_size() + get_t2_size() >= capacity)
            {
                replace(hit_in_ghost_b2);
            }

            Page page;
            disk.read(page, page_id);

            t2.push_front({page_id, page, false, false, T2});
            page_table[page_id] = {t2.begin(), T2};

            return t2.front().page;
        }

        Page page;
        disk.read(page, page_id);

        if (get_t1_size() + get_t2_size() >= capacity)
            replace();

        t1.push_front({page_id, page, false, false, T1});
        page_table[page_id] = {t1.begin(), T1};

        return t1.front().page;
    }

    void put(int page_id, const Page &page)
    {
        auto pt_it = page_table.find(page_id);

        if (pt_it != page_table.end())
        {
            auto &entry = pt_it->second;
            auto frame_it = entry.frame_it;

            frame_it->page = page;
            frame_it->dirty = true;
            frame_it->is_deleted = false;

            if (entry.status == T1)
            {
                Frame frame = *frame_it;
                frame.status = T2;
                t1.erase(frame_it);

                t2.push_front(frame);
                entry.frame_it = t2.begin();
                entry.status = T2;
            }
            else if (entry.status == T2)
            {
                t2.splice(t2.begin(), t2, frame_it);
                entry.frame_it = t2.begin();
            }
        }
        else
        {
            auto gt_it = ghost_table.find(page_id);
            bool hit_in_ghost_b2 = false;
            if (gt_it != ghost_table.end())
            {
                if (gt_it->second.status == B1)
                {
                    int delta =
                            (get_b1_size() >= get_b2_size() || get_b2_size() == 0) ? 1 : get_b2_size() / get_b1_size();
                    p = std::min(capacity, p + delta);
                    b1.erase(gt_it->second.ghost_it);
                }
                else
                {
                    int delta =
                            (get_b2_size() >= get_b1_size() || get_b1_size() == 0) ? 1 : get_b1_size() / get_b2_size();
                    p = std::max(0, p - delta);
                    b2.erase(gt_it->second.ghost_it);
                    hit_in_ghost_b2 = true;
                }
                ghost_table.erase(page_id);

                if (get_t1_size() + get_t2_size() >= capacity)
                    replace(hit_in_ghost_b2);

                t2.push_front({page_id, page, true, false, T2});
                page_table[page_id] = {t2.begin(), T2};
            }
            else
            {
                if (get_t1_size() + get_t2_size() >= capacity)
                    replace();

                t1.push_front({page_id, page, true, false, T1});
                page_table[page_id] = {t1.begin(), T1};
            }
        }
    }

    void Delete(int page_id)
    {
        auto pt_it = page_table.find(page_id);

        if (pt_it != page_table.end())
        {
            auto &entry = pt_it->second;
            if (entry.status == T1)
            {
                t1.erase(entry.frame_it);
            }
            else if (entry.status == T2)
            {
                t2.erase(entry.frame_it);
            }
            page_table.erase(pt_it);
        }

        auto gt_it = ghost_table.find(page_id);
        if (gt_it != ghost_table.end())
        {
            if (gt_it->second.status == B1)
            {
                b1.erase(gt_it->second.ghost_it);
            }
            else
            {
                b2.erase(gt_it->second.ghost_it);
            }
            ghost_table.erase(gt_it);
        }

        disk.Delete(page_id);
    }

    int new_page(Page &page)
    {
        int pos = disk.write(page);

        if (get_t1_size() + get_t2_size() >= capacity)
            replace();

        t1.push_front({pos, page, false, false, T1});
        page_table[pos] = {t1.begin(), T1};

        return pos;
    }

    void flush_all()
    {
        for (auto it = t1.begin(); it != t1.end();)
        {
            if (it->is_deleted)
            {
                disk.Delete(it->page_id);
                page_table.erase(it->page_id);
                it = t1.erase(it);
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

        for (auto it = t2.begin(); it != t2.end();)
        {
            if (it->is_deleted)
            {
                disk.Delete(it->page_id);
                page_table.erase(it->page_id);
                it = t2.erase(it);
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
