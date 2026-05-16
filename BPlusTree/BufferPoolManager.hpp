#ifndef BUFFERPOOLMANAGER_HPP
#define BUFFERPOOLMANAGER_HPP
#include "BPT_MemoryRiver.hpp"
#include "list.hpp"
#include "unordered_map.hpp"

template<class Page>
class BufferPoolManager
{
public:
    enum class AccessType
    {
        Unknown,
        Lookup,
        Scan,
        Index
    };

private:
    enum ArcStatus
    {
        T1, // Recent cache list (mru)
        T2, // Frequent cache list (mfu)
        B1, // Ghost list from T1
        B2 // Ghost list from T2
    };
    static constexpr int K_SCAN_PROMOTION_THRESHOLD = 3;

    struct Frame
    {
        int page_id;
        Page page;
        bool dirty = false;
        bool is_deleted = false;
        ArcStatus status = T1;
        AccessType access_type = AccessType::Unknown;
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

    // scan counter: page_id -> number of consecutive Scan accesses
    sjtu::unordered_map<int, int> scan_counter_;

    int p = 0;

    MemoryRiver<Page> &disk;

    int get_t1_size() const { return t1.size(); }
    int get_t2_size() const { return t2.size(); }
    int get_b1_size() const { return b1.size(); }
    int get_b2_size() const { return b2.size(); }

    bool has_evictable(sjtu::list<Frame> &list) const
    {
        for (auto it = list.begin(); it != list.end(); ++it)
        {
            if (it->page_id != root_pos)
                return true;
        }
        return false;
    }

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
            if (has_evictable(t1))
            {
                evict_from_list(t1, B1, b1);
                return;
            }
            if (has_evictable(t2))
            {
                evict_from_list(t2, B2, b2);
                return;
            }
        }
        else
        {
            if (has_evictable(t2))
            {
                evict_from_list(t2, B2, b2);
                return;
            }
            if (has_evictable(t1))
            {
                evict_from_list(t1, B1, b1);
                return;
            }
        }

        throw std::runtime_error("no evictable page (only root pinned?)");
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
            bool is_scan = (it->access_type == AccessType::Scan);

            if (it->dirty && !it->is_deleted)
                disk.update(it->page, pid);

            if (it->is_deleted)
                disk.Delete(pid);

            // Scan pages are not added to ghost lists
            if (!it->is_deleted && !is_scan)
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

            scan_counter_.erase(pid);
            page_table.erase(pid);
            list.erase(it);
            return;
        }

        throw std::runtime_error("no evictable page");
    }

public:
    BufferPoolManager(int cap, MemoryRiver<Page> &mr, int &root) : capacity(cap), disk(mr), root_pos(root), p(0) {}

    Page &get(int page_id, AccessType access_type)
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

                if (access_type == AccessType::Scan)
                {
                    // Window-based promotion for Scan accesses:
                    // increment counter, promote to T2 only if threshold reached.
                    auto &cnt = scan_counter_[page_id];
                    cnt++;
                    if (cnt >= K_SCAN_PROMOTION_THRESHOLD)
                    {
                        Frame frame = *frame_it;
                        frame.status = T2;
                        frame.access_type = access_type;
                        t1.erase(frame_it);

                        t2.push_front(frame);
                        entry.frame_it = t2.begin();
                        entry.status = T2;

                        scan_counter_.erase(page_id);
                        return t2.front().page;
                    }
                    // Not enough consecutive Scan accesses yet, stay in T1
                    frame_it->access_type = access_type;
                    return frame_it->page;
                }
                else
                {
                    // Lookup, Index, or Unknown: immediate promotion to T2
                    Frame frame = *frame_it;
                    frame.status = T2;
                    frame.access_type = access_type;
                    t1.erase(frame_it);

                    t2.push_front(frame);
                    entry.frame_it = t2.begin();
                    entry.status = T2;

                    scan_counter_.erase(page_id);
                    return t2.front().page;
                }
            }
            else if (entry.status == T2)
            {
                auto frame_it = entry.frame_it;
                if (frame_it->is_deleted)
                    throw std::runtime_error("access deleted page");

                t2.splice(t2.begin(), t2, frame_it);
                entry.frame_it = t2.begin();
                frame_it->access_type = access_type;

                return t2.front().page;
            }
        }

        auto gt_it = ghost_table.find(page_id);
        bool hit_in_ghost_b2 = false;
        bool is_scan = (access_type == AccessType::Scan);

        if (gt_it != ghost_table.end())
        {
            auto ghost_entry = gt_it->second;

            if (ghost_entry.status == B1)
            {
                if (!is_scan)
                {
                    int delta =
                            (get_b1_size() >= get_b2_size() || get_b2_size() == 0) ? 1 : get_b2_size() / get_b1_size();
                    p = std::min(capacity, p + delta);
                }
                b1.erase(ghost_entry.ghost_it);
            }
            else if (ghost_entry.status == B2)
            {
                if (!is_scan)
                {
                    int delta =
                            (get_b2_size() >= get_b1_size() || get_b1_size() == 0) ? 1 : get_b1_size() / get_b2_size();
                    p = std::max(0, p - delta);
                }
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

            if (is_scan)
            {
                // Scan access from ghost: insert into T1 with scan counter
                t1.push_front({page_id, page, false, false, T1, access_type});
                page_table[page_id] = {t1.begin(), T1};
                scan_counter_[page_id] = 1;
                return t1.front().page;
            }
            else
            {
                // Non-scan access from ghost: insert into T2
                t2.push_front({page_id, page, false, false, T2, access_type});
                page_table[page_id] = {t2.begin(), T2};
                return t2.front().page;
            }
        }

        Page page;
        disk.read(page, page_id);

        if (get_t1_size() + get_t2_size() >= capacity)
            replace();

        if (access_type == AccessType::Lookup || access_type == AccessType::Index)
        {
            // Point lookup / Index probe: insert directly into T2 (MFU)
            t2.push_front({page_id, page, false, false, T2, access_type});
            page_table[page_id] = {t2.begin(), T2};
            return t2.front().page;
        }
        else
        {
            // Unknown or Scan: insert into T1 (MRU)
            t1.push_front({page_id, page, false, false, T1, access_type});
            page_table[page_id] = {t1.begin(), T1};
            if (access_type == AccessType::Scan)
            {
                scan_counter_[page_id] = 1;
            }
            return t1.front().page;
        }
    }

    void put(int page_id, const Page &page, AccessType access_type)
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
                if (access_type == AccessType::Scan)
                {
                    // Window-based promotion for Scan accesses
                    auto &cnt = scan_counter_[page_id];
                    cnt++;
                    if (cnt >= K_SCAN_PROMOTION_THRESHOLD)
                    {
                        Frame frame = *frame_it;
                        frame.status = T2;
                        frame.access_type = access_type;
                        t1.erase(frame_it);

                        t2.push_front(frame);
                        entry.frame_it = t2.begin();
                        entry.status = T2;

                        scan_counter_.erase(page_id);
                    }
                    else
                    {
                        frame_it->access_type = access_type;
                    }
                }
                else
                {
                    Frame frame = *frame_it;
                    frame.status = T2;
                    frame.access_type = access_type;
                    t1.erase(frame_it);

                    t2.push_front(frame);
                    entry.frame_it = t2.begin();
                    entry.status = T2;

                    scan_counter_.erase(page_id);
                }
            }
            else if (entry.status == T2)
            {
                t2.splice(t2.begin(), t2, frame_it);
                entry.frame_it = t2.begin();
                frame_it->access_type = access_type;
            }
        }
        else
        {
            auto gt_it = ghost_table.find(page_id);
            bool hit_in_ghost_b2 = false;
            bool is_scan = (access_type == AccessType::Scan);

            if (gt_it != ghost_table.end())
            {
                if (gt_it->second.status == B1)
                {
                    if (!is_scan)
                    {
                        int delta = (get_b1_size() >= get_b2_size() || get_b2_size() == 0)
                                            ? 1
                                            : get_b2_size() / get_b1_size();
                        p = std::min(capacity, p + delta);
                    }
                    b1.erase(gt_it->second.ghost_it);
                }
                else
                {
                    if (!is_scan)
                    {
                        int delta = (get_b2_size() >= get_b1_size() || get_b1_size() == 0)
                                            ? 1
                                            : get_b1_size() / get_b2_size();
                        p = std::max(0, p - delta);
                    }
                    b2.erase(gt_it->second.ghost_it);
                    hit_in_ghost_b2 = true;
                }
                ghost_table.erase(page_id);

                if (get_t1_size() + get_t2_size() >= capacity)
                    replace(hit_in_ghost_b2);

                if (is_scan)
                {
                    t1.push_front({page_id, page, true, false, T1, access_type});
                    page_table[page_id] = {t1.begin(), T1};
                    scan_counter_[page_id] = 1;
                }
                else
                {
                    t2.push_front({page_id, page, true, false, T2, access_type});
                    page_table[page_id] = {t2.begin(), T2};
                }
            }
            else
            {
                if (get_t1_size() + get_t2_size() >= capacity)
                    replace();

                if (access_type == AccessType::Lookup || access_type == AccessType::Index)
                {
                    t2.push_front({page_id, page, true, false, T2, access_type});
                    page_table[page_id] = {t2.begin(), T2};
                }
                else
                {
                    t1.push_front({page_id, page, true, false, T1, access_type});
                    page_table[page_id] = {t1.begin(), T1};
                    if (access_type == AccessType::Scan)
                    {
                        scan_counter_[page_id] = 1;
                    }
                }
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
            scan_counter_.erase(page_id);
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

        t1.push_front({pos, page, false, false, T1, AccessType::Unknown});
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
                scan_counter_.erase(it->page_id);
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
                scan_counter_.erase(it->page_id);
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
