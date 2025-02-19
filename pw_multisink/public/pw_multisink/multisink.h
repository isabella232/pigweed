// Copyright 2021 The Pigweed Authors
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.
#pragma once

#include <mutex>

#include "pw_bytes/span.h"
#include "pw_multisink/config.h"
#include "pw_result/result.h"
#include "pw_ring_buffer/prefixed_entry_ring_buffer.h"
#include "pw_status/status.h"
#include "pw_sync/lock_annotations.h"

namespace pw {
namespace multisink {

// An asynchronous single-writer multi-reader queue that ensures readers can
// poll for dropped message counts, which is useful for logging or similar
// scenarios where readers need to be aware of the input message sequence.
//
// This class is thread-safe but NOT IRQ-safe when
// PW_MULTISINK_LOCK_INTERRUPT_SAFE is disabled.
class MultiSink {
 public:
  // An asynchronous reader which is attached to a MultiSink via AttachDrain.
  // Each Drain holds a PrefixedEntryRingBufferMulti::Reader and abstracts away
  // entry sequence information for clients.
  class Drain {
   public:
    constexpr Drain() : last_handled_sequence_id_(0), multisink_(nullptr) {}

    // Returns the next available entry if it exists and acquires the latest
    // drop count in parallel.
    //
    // The `drop_count_out` is set to the number of entries that were dropped
    // since the last call to GetEntry, if the read operation was successful or
    // returned OutOfRange (i.e. no entries to read). Otherwise, it is set to
    // zero, so should always be processed.
    //
    // Drop counts are internally maintained with a 32-bit counter. If
    // UINT32_MAX entries have been handled by the attached multisink between
    // subsequent calls to GetEntry, the drop count will overflow and will
    // report a lower count erroneously. Users should ensure that sinks call
    // GetEntry at least once every UINT32_MAX entries.
    //
    // Example Usage:
    //
    // void ProcessEntriesFromDrain(Drain& drain) {
    //   std::array<std::byte, kEntryBufferSize> buffer;
    //   uint32_t drop_count = 0;
    //
    //   // Example#1: Request the drain for a new entry.
    //   {
    //     const Result<ConstByteSpan> result = drain.GetEntry(buffer,
    //                                                         drop_count);
    //
    //     // If a non-zero drop count is received, process them.
    //     if (drop_count > 0) {
    //       ProcessDropCount(drop_count);
    //     }
    //
    //     // If the call was successful, process the entry that was received.
    //     if (result.ok()) {
    //       ProcessEntry(result.value());
    //     }
    //   }
    //
    //   // Example#2: Drain out all messages.
    //   {
    //     Result<ConstByteSpan> result = Status::OutOfRange();
    //     do {
    //       result = drain.GetEntry(buffer, drop_count);
    //
    //       if (drop_count > 0) {
    //         ProcessDropCount(drop_count);
    //       }
    //
    //       if (result.ok()) {
    //         ProcessEntry(result.value());
    //       }
    //
    //       // Keep trying until we hit OutOfRange. Note that a new entry may
    //       // have arrived after the GetEntry call.
    //     } while (!result.IsOutOfRange());
    //   }
    // }
    //
    // Return values:
    // Ok - An entry was successfully read from the multisink.
    // OutOfRange - No entries were available.
    // FailedPrecondition - The drain must be attached to a sink.
    // ResourceExhausted - The provided buffer was not large enough to store
    // the next available entry.
    // DataLoss - An entry was read but did not match the expected format.
    Result<ConstByteSpan> GetEntry(ByteSpan buffer, uint32_t& drop_count_out)
        PW_LOCKS_EXCLUDED(multisink_->lock_);

    // Drains are not copyable or movable.
    Drain(const Drain&) = delete;
    Drain& operator=(const Drain&) = delete;
    Drain(Drain&&) = delete;
    Drain& operator=(Drain&&) = delete;

   protected:
    friend MultiSink;

    // The `reader_` and `last_handled_sequence_id_` are managed by attached
    // multisink and are guarded by `multisink_->lock_` when used.
    ring_buffer::PrefixedEntryRingBufferMulti::Reader reader_;
    uint32_t last_handled_sequence_id_;
    MultiSink* multisink_;
  };

  // A pure-virtual listener of a MultiSink, attached via AttachListener.
  // MultiSink's invoke listeners when new data arrives, allowing them to
  // schedule the draining of messages out of the MultiSink.
  class Listener : public IntrusiveList<Listener>::Item {
   public:
    constexpr Listener() {}
    virtual ~Listener() = default;

    // Listeners are not copyable or movable.
    Listener(const Listener&) = delete;
    Listener& operator=(const Drain&) = delete;
    Listener(Listener&&) = delete;
    Listener& operator=(Drain&&) = delete;

   protected:
    friend MultiSink;

    // Invoked by the attached multisink when a new entry or drop count is
    // available. The multisink lock is held during this call, so neither the
    // multisink nor it's drains can be used during this callback.
    virtual void OnNewEntryAvailable() = 0;
  };

  // Constructs a multisink using a ring buffer backed by the provided buffer.
  MultiSink(ByteSpan buffer) : ring_buffer_(true), sequence_id_(0) {
    ring_buffer_.SetBuffer(buffer);
  }

  // Write an entry to the multisink. If available space is less than the
  // size of the entry, the internal ring buffer will push the oldest entries
  // out to make space, so long as the entry is not larger than the buffer.
  // The sequence ID of the multisink will always increment as a result of
  // calling HandleEntry, regardless of whether pushing the entry succeeds.
  //
  // Precondition: If PW_MULTISINK_LOCK_INTERRUPT_SAFE is disabled, this
  // function must not be called from an interrupt context.
  // Precondition: entry.size() > 0
  // Precondition: entry.size() <= `ring_buffer_` size
  void HandleEntry(ConstByteSpan entry) PW_LOCKS_EXCLUDED(lock_);

  // Notifies the multisink of messages dropped before ingress. The writer
  // may use this to signal to readers that an entry (or entries) failed
  // before being sent to the multisink (e.g. the writer failed to encode
  // the message). This API increments the sequence ID of the multisink by
  // the provided `drop_count`.
  void HandleDropped(uint32_t drop_count = 1) PW_LOCKS_EXCLUDED(lock_);

  // Attach a drain to the multisink. Drains may not be associated with more
  // than one multisink at a time. Entries pushed before the drain was attached
  // are not seen by the drain, so drains should be attached before entries
  // are pushed.
  //
  // Precondition: The drain must not be attached to a multisink.
  void AttachDrain(Drain& drain) PW_LOCKS_EXCLUDED(lock_);

  // Detaches a drain from the multisink. Drains may only be detached if they
  // were previously attached to this multisink.
  //
  // Precondition: The drain must be attached to this multisink.
  void DetachDrain(Drain& drain) PW_LOCKS_EXCLUDED(lock_);

  // Attach a listener to the multisink. Entries pushed before the listener was
  // attached are not seen by the listener, so listeners should be attached
  // before entries are pushed. Listeners are invoked on all new messages.
  //
  // Precondition: The listener must not be attached to a multisink.
  void AttachListener(Listener& listener) PW_LOCKS_EXCLUDED(lock_);

  // Detaches a listener from the multisink.
  //
  // Precondition: The listener must be attached to this multisink.
  void DetachListener(Listener& listener) PW_LOCKS_EXCLUDED(lock_);

  // Removes all data from the internal buffer. The multisink's sequence ID is
  // not modified, so readers may interpret this event as droppping entries.
  void Clear() PW_LOCKS_EXCLUDED(lock_);

 protected:
  friend Drain;
  // Gets an entry from the provided drain and unpacks sequence ID information.
  // Drains use this API to strip away sequence ID information for drop
  // calculation.
  //
  // Returns:
  // Ok - An entry was successfully read from the multisink. The
  // `drop_count_out` is set to the difference between the current sequence ID
  // and the last handled ID. FailedPrecondition - The drain is not attached to
  // a multisink. ResourceExhausted - The provided buffer was not large enough
  // to store the next available entry. DataLoss - An entry was read from the
  // multisink, but did not contains an encoded sequence ID.
  Result<ConstByteSpan> GetEntry(Drain& drain,
                                 ByteSpan buffer,
                                 uint32_t& drop_count_out)
      PW_LOCKS_EXCLUDED(lock_);

 private:
  // Notifies attached listeners of new entries or an updated drop count.
  void NotifyListeners() PW_EXCLUSIVE_LOCKS_REQUIRED(lock_);

  IntrusiveList<Listener> listeners_ PW_GUARDED_BY(lock_);
  ring_buffer::PrefixedEntryRingBufferMulti ring_buffer_ PW_GUARDED_BY(lock_);
  uint32_t sequence_id_ PW_GUARDED_BY(lock_);
  LockType lock_;
};

}  // namespace multisink
}  // namespace pw
