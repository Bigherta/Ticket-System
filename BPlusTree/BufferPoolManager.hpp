#ifndef BUFFERPOOLMANAGER_HPP
#define BUFFERPOOLMANAGER_HPP
#include "BPT_MemoryRiver.hpp"
#include "unordered_map.hpp"
#include "vector.hpp"
#include "arc_replacer.hpp"

template<class Page>
class BufferPoolManager
{
private:
    struct Frame
    {
        int page_id = -1;
        Page *page = nullptr;
        bool dirty = false;
        bool is_deleted = false;
    };

    int &root_pos;
    int capacity;

    sjtu::vector<Frame> frames_;
    sjtu::vector<int> free_frames_;
    sjtu::unordered_map<int, int> page_table_; // page_id -> frame_id

    MemoryRiver<Page> &disk;
    sjtu::ArcReplacer replacer_;

    static constexpr size_t K_SCAN_PROMOTION_THRESHOLD = 3;

    int get_free_frame()
    {
        if (!free_frames_.empty()) {
            int fid = free_frames_.back();
            free_frames_.pop_back();
            return fid;
        }
        auto fid = replacer_.Evict();
        if (fid == -1) {
            return -1;
        }
        return fid;
    }

    void write_back_if_needed(int fid)
    {
        auto &f = frames_[fid];
        if (f.page && f.page_id != -1) {
            if (f.dirty && !f.is_deleted) {
                disk.update(*f.page, f.page_id);
            }
            if (f.is_deleted) {
                disk.Delete(f.page_id);
            }
            page_table_.erase(f.page_id);
            delete f.page;
            f.page = nullptr;
            f.page_id = -1;
            f.dirty = false;
            f.is_deleted = false;
        }
    }

public:
    BufferPoolManager(int cap, MemoryRiver<Page> &mr, int &root) : root_pos(root), capacity(cap), disk(mr), replacer_(cap) {
        frames_.reserve(capacity);
        for (int i = 0; i < capacity; ++i) {
            frames_.push_back(Frame());
            free_frames_.push_back(i);
        }
    }

    ~BufferPoolManager() { flush_all(); }

    Page &get(int page_id, sjtu::AccessType access_type)
    {
        auto it = page_table_.find(page_id);
        if (it != page_table_.end()) {
            int fid = it->second;
            replacer_.RecordAccess(fid, page_id, access_type);
            return *frames_[fid].page;
        }

        int fid = get_free_frame();
        if (fid == -1) {
            throw std::runtime_error("no frame available");
        }

        // If this frame had content, write back and clear
        write_back_if_needed(fid);

        Page *p = new Page();
        disk.read(*p, page_id);
        frames_[fid].page = p;
        frames_[fid].page_id = page_id;
        frames_[fid].dirty = false;
        frames_[fid].is_deleted = false;
        page_table_[page_id] = fid;

        replacer_.RecordAccess(fid, page_id, access_type);
        return *p;
    }

    void put(int page_id, const Page &page, sjtu::AccessType access_type)
    {
        auto it = page_table_.find(page_id);
        if (it != page_table_.end()) {
            int fid = it->second;
            if (!frames_[fid].page) frames_[fid].page = new Page(page);
            else *frames_[fid].page = page;
            frames_[fid].dirty = true;
            replacer_.RecordAccess(fid, page_id, access_type);
            return;
        }

        int fid = get_free_frame();
        if (fid == -1) throw std::runtime_error("no frame available");

        write_back_if_needed(fid);

        Page *p = new Page(page);
        frames_[fid].page = p;
        frames_[fid].page_id = page_id;
        frames_[fid].dirty = true;
        frames_[fid].is_deleted = false;
        page_table_[page_id] = fid;

        replacer_.RecordAccess(fid, page_id, access_type);
    }

    void Delete(int page_id)
    {
        auto it = page_table_.find(page_id);
        if (it != page_table_.end()) {
            int fid = it->second;
            frames_[fid].is_deleted = true;
            write_back_if_needed(fid);
            page_table_.erase(page_id);
            free_frames_.push_back(fid);
        }
        // Ensure deleted on disk
        disk.Delete(page_id);
    }

    int new_page(Page &page)
    {
        int pos = disk.write(page);

        int fid = get_free_frame();
        if (fid == -1) throw std::runtime_error("no frame available");

        write_back_if_needed(fid);

        Page *p = new Page(page);
        frames_[fid].page = p;
        frames_[fid].page_id = pos;
        frames_[fid].dirty = false;
        frames_[fid].is_deleted = false;
        page_table_[pos] = fid;

        replacer_.RecordAccess(fid, pos, sjtu::AccessType::Lookup);
        return pos;
    }

    void flush_all()
    {
        for (const auto &kv : page_table_) {
            int pid = kv.first;
            int fid = kv.second;
            if (frames_[fid].page && frames_[fid].dirty) {
                disk.update(*frames_[fid].page, pid);
                frames_[fid].dirty = false;
            }
        }
    }
};

#endif // BUFFERPOOLMANAGER_HPP
