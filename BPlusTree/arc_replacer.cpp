// ARC replacer implementation moved from header

#include "arc_replacer.hpp"
#include "exceptions.hpp"

namespace sjtu {

ArcReplacer::ArcReplacer(size_t num_frames) : replacer_size_(num_frames) {}

auto ArcReplacer::Evict() -> frame_id_t {
  if (mru_.size() >= std::max<size_t>(1, mru_target_size_)) {
    for (auto it = mru_.rbegin(); it != mru_.rend(); ++it) {
      auto frame_id = *it;
      auto frame_status_ptr = alive_map_[frame_id];
      if (frame_status_ptr->evictable_) {
        if (frame_status_ptr->access_type_ != AccessType::Scan) {
          mru_ghost_.push_front(frame_status_ptr->page_id_);
          page_list_map_[frame_status_ptr->page_id_] = mru_ghost_.begin();
          ghost_map_[frame_status_ptr->page_id_] = shared_ptr<FrameStatus>(new FrameStatus(frame_status_ptr->page_id_, frame_id, false, ArcStatus::MRU_GHOST));
        }
        // Clean up scan counter if present
        scan_counter_.erase(frame_status_ptr->page_id_);
        {
          auto victim = it.base();
          --victim;
          mru_.erase(victim);
        }
        frame_list_map_.erase(frame_id);
        alive_map_.erase(frame_id);
        curr_size_--;
        return frame_id;
      }
    }
    for (auto it = mfu_.rbegin(); it != mfu_.rend(); ++it) {
      auto frame_id = *it;
      auto frame_status_ptr = alive_map_[frame_id];
      if (frame_status_ptr->evictable_) {
        if (frame_status_ptr->access_type_ != AccessType::Scan) {
          mfu_ghost_.push_front(frame_status_ptr->page_id_);
          page_list_map_[frame_status_ptr->page_id_] = mfu_ghost_.begin();
          ghost_map_[frame_status_ptr->page_id_] = shared_ptr<FrameStatus>(new FrameStatus(frame_status_ptr->page_id_, frame_id, false, ArcStatus::MFU_GHOST));
        }
        scan_counter_.erase(frame_status_ptr->page_id_);
        {
          auto victim = it.base();
          --victim;
          mfu_.erase(victim);
        }
        frame_list_map_.erase(frame_id);
        alive_map_.erase(frame_id);
        curr_size_--;
        return frame_id;
      }
    }
  } else {
    for (auto it = mfu_.rbegin(); it != mfu_.rend(); ++it) {
      auto frame_id = *it;
      auto frame_status_ptr = alive_map_[frame_id];
      if (frame_status_ptr->evictable_) {
        if (frame_status_ptr->access_type_ != AccessType::Scan) {
          mfu_ghost_.push_front(frame_status_ptr->page_id_);
          page_list_map_[frame_status_ptr->page_id_] = mfu_ghost_.begin();
          ghost_map_[frame_status_ptr->page_id_] = shared_ptr<FrameStatus>(new FrameStatus(frame_status_ptr->page_id_, frame_id, false, ArcStatus::MFU_GHOST));
        }
        scan_counter_.erase(frame_status_ptr->page_id_);
        {
          auto victim = it.base();
          --victim;
          mfu_.erase(victim);
        }
        frame_list_map_.erase(frame_id);
        alive_map_.erase(frame_id);
        curr_size_--;
        return frame_id;
      }
    }
    for (auto it = mru_.rbegin(); it != mru_.rend(); ++it) {
      auto frame_id = *it;
      auto frame_status_ptr = alive_map_[frame_id];
      if (frame_status_ptr->evictable_) {
        if (frame_status_ptr->access_type_ != AccessType::Scan) {
          mru_ghost_.push_front(frame_status_ptr->page_id_);
          page_list_map_[frame_status_ptr->page_id_] = mru_ghost_.begin();
          ghost_map_[frame_status_ptr->page_id_] = shared_ptr<FrameStatus>(new FrameStatus(frame_status_ptr->page_id_, frame_id, false, ArcStatus::MRU_GHOST));
        }
        scan_counter_.erase(frame_status_ptr->page_id_);
        {
          auto victim = it.base();
          --victim;
          mru_.erase(victim);
        }
        frame_list_map_.erase(frame_id);
        alive_map_.erase(frame_id);
        curr_size_--;
        return frame_id;
      }
    }
  }
  return -1;
}

void ArcReplacer::RecordAccess(frame_id_t frame_id, page_id_t page_id, [[maybe_unused]] AccessType access_type) {
  const bool is_scan = access_type == AccessType::Scan;
  const bool is_lookup = access_type == AccessType::Lookup;
  const bool is_index = access_type == AccessType::Index;

  auto alive_it = alive_map_.find(frame_id);
  if (alive_it != alive_map_.end()) {
    auto list_it = frame_list_map_.at(frame_id);
    if (alive_it->second->arc_status_ == ArcStatus::MRU) {
      if (is_scan) {
        // Window-based promotion for Scan accesses:
        // increment counter, and promote to MFU only if threshold is reached.
        auto &cnt = scan_counter_[page_id];
        cnt++;
        if (cnt >= K_SCAN_PROMOTION_THRESHOLD) {
          mfu_.splice(mfu_.begin(), mru_, list_it);
          frame_list_map_[frame_id] = mfu_.begin();
          alive_it->second->arc_status_ = ArcStatus::MFU;
          scan_counter_.erase(page_id);
        }
      } else {
        // Index or Lookup: immediate promotion to MFU
        mfu_.splice(mfu_.begin(), mru_, list_it);
        frame_list_map_[frame_id] = mfu_.begin();
        alive_it->second->arc_status_ = ArcStatus::MFU;
        scan_counter_.erase(page_id);
      }
    } else {
      // Already in MFU: move to front
      mfu_.splice(mfu_.begin(), mfu_, list_it);
      frame_list_map_[frame_id] = mfu_.begin();
    }
    alive_it->second->access_type_ = access_type;
    return;
  }
  auto ghost_it = ghost_map_.find(page_id);
  if (ghost_it != ghost_map_.end()) {
    auto list_it = page_list_map_[page_id];
    if (ghost_it->second->arc_status_ == ArcStatus::MRU_GHOST) {
      if (!is_scan && mru_target_size_ < replacer_size_) {
        if (mru_ghost_.size() >= mfu_ghost_.size()) {
          mru_target_size_++;
        } else {
          mru_target_size_ += std::min(mfu_ghost_.size() / mru_ghost_.size(), replacer_size_ - mru_target_size_);
        }
      }
      mru_ghost_.erase(list_it);
    } else {
      if (!is_scan && mru_target_size_ > 0) {
        if (mfu_ghost_.size() >= mru_ghost_.size()) {
          mru_target_size_--;
        } else {
          mru_target_size_ -= std::min(mru_ghost_.size() / mfu_ghost_.size(), mru_target_size_);
        }
      }
      mfu_ghost_.erase(list_it);
    }
    page_list_map_.erase(page_id);
    if (is_scan) {
      mru_.push_front(frame_id);
      frame_list_map_[frame_id] = mru_.begin();
      alive_map_[frame_id] = shared_ptr<FrameStatus>(new FrameStatus(page_id, frame_id, false, ArcStatus::MRU, access_type));
      scan_counter_[page_id] = 1;
    } else {
      mfu_.push_front(frame_id);
      frame_list_map_[frame_id] = mfu_.begin();
      alive_map_[frame_id] = shared_ptr<FrameStatus>(new FrameStatus(page_id, frame_id, false, ArcStatus::MFU, access_type));
    }
    ghost_map_.erase(page_id);
    return;
  }
  if (mru_.size() + mru_ghost_.size() == replacer_size_) {
    if (!mru_ghost_.empty()) {
      auto it = mru_ghost_.end();
      --it;
      auto page = *it;

      mru_ghost_.erase(it);
      page_list_map_.erase(page);
      ghost_map_.erase(page);
    }
  } else if (mru_.size() + mru_ghost_.size() + mfu_.size() + mfu_ghost_.size() == 2 * replacer_size_) {
    if (!mfu_ghost_.empty()) {
      auto it = mfu_ghost_.end();
      --it;
      auto page = *it;

      mfu_ghost_.erase(it);
      page_list_map_.erase(page);
      ghost_map_.erase(page);
    }
  }
  if (!is_lookup && !is_index) {
    mru_.push_front(frame_id);
    frame_list_map_[frame_id] = mru_.begin();
    alive_map_[frame_id] = shared_ptr<FrameStatus>(new FrameStatus(page_id, frame_id, false, ArcStatus::MRU, access_type));
    if (is_scan) {
      scan_counter_[page_id] = 1;
    }
  } else {
    mfu_.push_front(frame_id);
    frame_list_map_[frame_id] = mfu_.begin();
    alive_map_[frame_id] = shared_ptr<FrameStatus>(new FrameStatus(page_id, frame_id, false, ArcStatus::MFU, access_type));
  }
}

void ArcReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  auto alive_it = alive_map_.find(frame_id);
  if (alive_it == alive_map_.end()) {
    throw sjtu::invalid_argument("invalid frame id");
  }
  if (alive_it->second->evictable_ && !set_evictable) {
    curr_size_--;
    alive_it->second->evictable_ = false;
  } else if (!alive_it->second->evictable_ && set_evictable) {
    curr_size_++;
    alive_it->second->evictable_ = true;
  }
}

void ArcReplacer::Remove(frame_id_t frame_id) {
  auto alive_it = alive_map_.find(frame_id);
  if (alive_it == alive_map_.end()) {
    return;
  }
  if (!alive_it->second->evictable_) {
    throw sjtu::invalid_argument("frame is not evictable");
  }
  auto list_it = frame_list_map_[frame_id];
  if (alive_it->second->arc_status_ == ArcStatus::MRU) {
    mru_.erase(list_it);
  } else {
    mfu_.erase(list_it);
  }
  scan_counter_.erase(alive_it->second->page_id_);
  frame_list_map_.erase(frame_id);
  alive_map_.erase(frame_id);
  curr_size_--;
}

auto ArcReplacer::Size() -> size_t {
  return curr_size_;
}

}  // namespace sjtu
