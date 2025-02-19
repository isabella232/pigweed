// Copyright 2020 The Pigweed Authors
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

#include <array>
#include <cstddef>
#include <span>

#include "pw_bytes/span.h"
#include "pw_result/result.h"
#include "pw_stream/stream.h"

namespace pw::stream {

class MemoryWriter : public Writer {
 public:
  constexpr MemoryWriter(ByteSpan dest) : dest_(dest) {}

  // Construct writer with prepopulated data in the buffer. The number of
  // pre-written bytes is provided as `bytes_written`.
  //
  // Precondition: The number of pre-written bytes must not be greater than the
  // size of the provided buffer.
  constexpr MemoryWriter(ByteSpan dest, size_t bytes_written)
      : dest_(dest), bytes_written_(bytes_written) {
    PW_ASSERT(bytes_written_ <= dest.size_bytes());
  }

  size_t bytes_written() const { return bytes_written_; }

  size_t ConservativeWriteLimit() const override {
    return dest_.size_bytes() - bytes_written_;
  }

  ConstByteSpan WrittenData() const { return dest_.first(bytes_written_); }

  std::byte* data() { return dest_.data(); }
  const std::byte* data() const { return dest_.data(); }

 private:
  // Implementation for writing data to this stream.
  //
  // If the in-memory buffer is exhausted in the middle of a write, this will
  // perform a partial write and Status::ResourceExhausted() will be returned.
  Status DoWrite(ConstByteSpan data) override;

  ByteSpan dest_;
  size_t bytes_written_ = 0;
};

template <size_t kSizeBytes>
class MemoryWriterBuffer final : public MemoryWriter {
 public:
  constexpr MemoryWriterBuffer() : MemoryWriter(buffer_) {}

 private:
  std::array<std::byte, kSizeBytes> buffer_;
};

class MemoryReader final : public Reader {
 public:
  constexpr MemoryReader(ConstByteSpan source)
      : source_(source), bytes_read_(0) {}

  size_t ConservativeReadLimit() const override {
    return source_.size_bytes() - bytes_read_;
  }

  size_t bytes_read() const { return bytes_read_; }

  const std::byte* data() const { return source_.data(); }

 private:
  // Implementation for reading data from this stream.
  //
  // If the in-memory buffer does not have enough remaining bytes for what was
  // requested, this will perform a partial read and OK will still be returned.
  StatusWithSize DoRead(ByteSpan dest) override;

  ConstByteSpan source_;
  size_t bytes_read_;
};

}  // namespace pw::stream
