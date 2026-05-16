#pragma once

#include "list.hpp"
#include "unordered_map.hpp"
#include "shared_ptr.hpp"

namespace sjtu {

enum class AccessType { Unknown = 0, Lookup, Scan, Index };

enum class ArcStatus { MRU, MFU, MRU_GHOST, MFU_GHOST };

using page_id_t = int;
using frame_id_t = int;

// TODO(student): You can modify or remove this struct as you like.
struct FrameStatus {
  page_id_t page_id_;
  frame_id_t frame_id_;
  bool evictable_;
  ArcStatus arc_status_;
  AccessType access_type_;
  FrameStatus(page_id_t pid, frame_id_t fid, bool ev, ArcStatus st, AccessType at = AccessType::Unknown)
      : page_id_(pid), frame_id_(fid), evictable_(ev), arc_status_(st), access_type_(at) {}
};

/**
 * ArcReplacer implements the ARC replacement policy.
 */
class ArcReplacer {
 public:
  explicit ArcReplacer(size_t num_frames);

  ArcReplacer(const ArcReplacer &) = delete;
  ArcReplacer(ArcReplacer &&) = delete;

  /**
   * TODO(P1): Add implementation
   *
   * @brief Destroys the LRUReplacer.
   */
  ~ArcReplacer() = default;

  // returns frame id, or -1 if cannot evict
  auto Evict() -> frame_id_t;
  void RecordAccess(frame_id_t frame_id, page_id_t page_id, AccessType access_type = AccessType::Unknown);
  void SetEvictable(frame_id_t frame_id, bool set_evictable);
  void Remove(frame_id_t frame_id);
  auto Size() -> size_t;

 private:
  // TODO(student): implement me! You can replace or remove these member variables as you like.
  sjtu::list<frame_id_t> mru_;
  sjtu::list<frame_id_t> mfu_;
  sjtu::list<page_id_t> mru_ghost_;
  sjtu::list<page_id_t> mfu_ghost_;

  /* record entries in mru_ and mfu_
   * this uses frame_id_t to guarantee no duplicate records for the same
   * frame when they are alive */
  sjtu::unordered_map<frame_id_t, shared_ptr<FrameStatus>> alive_map_;
  /* record entries in mru_ghost_ and mfu_ghost_
   * this uses page_id_t but not frame_id_t because page_id is the unique
   * identifier in ghost lists */
  sjtu::unordered_map<page_id_t, shared_ptr<FrameStatus>> ghost_map_;

  /* alive, evictable entries count */
  size_t curr_size_{0};
  /* p as in original paper */
  size_t mru_target_size_{0};
  /* c as in original paper */
  size_t replacer_size_;

  // maps to list iterators for quick erase/splice
  sjtu::unordered_map<frame_id_t, typename sjtu::list<frame_id_t>::iterator> frame_list_map_;
  sjtu::unordered_map<page_id_t, typename sjtu::list<page_id_t>::iterator> page_list_map_;

  /**
   * @brief Per-page scan access counter for window-based promotion.
   *
   * When a Scan access hits an alive page in MRU, the counter increments.
   * Once the counter reaches K_SCAN_PROMOTION_THRESHOLD, the page is promoted
   * to MFU and the counter is cleared. This prevents scan pollution while
   * still allowing frequently re-accessed Scan pages to be retained.
   */
  static constexpr size_t K_SCAN_PROMOTION_THRESHOLD = 3;
  sjtu::unordered_map<page_id_t, size_t> scan_counter_;
};

}  // namespace sjtu