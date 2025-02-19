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

#include <cstddef>
#include <new>
#include <utility>

#include "pw_assert/assert.h"
#include "pw_function/config.h"
#include "pw_preprocessor/compiler.h"

namespace pw::function_internal {

template <typename T, typename Comparison = bool>
struct NullEq {
  static constexpr bool Test(const T&) { return false; }
};

// Partial specialization for values of T comparable to nullptr.
template <typename T>
struct NullEq<T, decltype(std::declval<T>() == nullptr)> {
  // This is intended to be used for comparing function pointers to nullptr, but
  // the specialization also matches Ts that implicitly convert to a function
  // pointer, such as function types. The compiler may then complain that the
  // comparison is false, as the address is known at compile time and cannot be
  // nullptr. Silence this warning. (The compiler will optimize out the
  // comparison.)
  PW_MODIFY_DIAGNOSTICS_PUSH();
  PW_MODIFY_DIAGNOSTIC(ignored, "-Waddress");
  static constexpr bool Test(const T& v) { return v == nullptr; }
  PW_MODIFY_DIAGNOSTICS_POP();
};

// Tests whether a value is considered to be null.
template <typename T>
static constexpr bool IsNull(const T& v) {
  return NullEq<T>::Test(v);
}

// FunctionTarget is an interface for storing a callable object and providing a
// way to invoke it.
template <typename Return, typename... Args>
class FunctionTarget {
 public:
  FunctionTarget() = default;
  virtual ~FunctionTarget() = default;

  FunctionTarget(const FunctionTarget&) = delete;
  FunctionTarget(FunctionTarget&&) = delete;
  FunctionTarget& operator=(const FunctionTarget&) = delete;
  FunctionTarget& operator=(FunctionTarget&&) = delete;

  virtual bool IsNull() const = 0;

  // Invoke the callable stored by the function target.
  virtual Return operator()(Args... args) const = 0;

  // Move initialize the function target to a provided location.
  virtual void MoveInitializeTo(void* ptr) = 0;
};

// A function target that does not store any callable. Attempting to invoke it
// results in a crash.
template <typename Return, typename... Args>
class NullFunctionTarget final : public FunctionTarget<Return, Args...> {
 public:
  NullFunctionTarget() = default;
  ~NullFunctionTarget() final = default;

  NullFunctionTarget(const NullFunctionTarget&) = delete;
  NullFunctionTarget(NullFunctionTarget&&) = delete;
  NullFunctionTarget& operator=(const NullFunctionTarget&) = delete;
  NullFunctionTarget& operator=(NullFunctionTarget&&) = delete;

  bool IsNull() const final { return true; }

  Return operator()(Args...) const final { PW_ASSERT(false); }

  void MoveInitializeTo(void* ptr) final { new (ptr) NullFunctionTarget(); }
};

// Function target that stores a callable as a member within the class.
template <typename Callable, typename Return, typename... Args>
class InlineFunctionTarget final : public FunctionTarget<Return, Args...> {
 public:
  explicit InlineFunctionTarget(Callable&& callable)
      : callable_(std::move(callable)) {}

  ~InlineFunctionTarget() final = default;

  InlineFunctionTarget(const InlineFunctionTarget&) = delete;
  InlineFunctionTarget& operator=(const InlineFunctionTarget&) = delete;

  InlineFunctionTarget(InlineFunctionTarget&& other)
      : callable_(std::move(other.callable_)) {}
  InlineFunctionTarget& operator=(InlineFunctionTarget&&) = default;

  bool IsNull() const final { return false; }

  Return operator()(Args... args) const final { return callable_(args...); }

  void MoveInitializeTo(void* ptr) final {
    new (ptr) InlineFunctionTarget(std::move(*this));
  }

 private:
  // This must be mutable to support custom objects that implement operator() in
  // a non-const way.
  mutable Callable callable_;
};

// Function target which stores a callable at a provided location in memory.
// The creating context must ensure that the region is properly sized and
// aligned for the callable.
template <typename Callable, typename Return, typename... Args>
class MemoryFunctionTarget final : public FunctionTarget<Return, Args...> {
 public:
  MemoryFunctionTarget(void* address, Callable&& callable) : address_(address) {
    new (address_) Callable(std::move(callable));
  }

  ~MemoryFunctionTarget() final {
    // Multiple MemoryFunctionTargets may have referred to the same callable
    // (due to moves), but only one can have a valid pointer to it. The owner is
    // responsible for destructing the callable.
    if (address_ != nullptr) {
      callable().~Callable();
    }
  }

  MemoryFunctionTarget(const MemoryFunctionTarget&) = delete;
  MemoryFunctionTarget& operator=(const MemoryFunctionTarget&) = delete;

  // Transfer the pointer to the initialized callable to this object without
  // reinitializing the callable, clearing the address from the other.
  MemoryFunctionTarget(MemoryFunctionTarget&& other)
      : address_(other.address_) {
    other.address_ = nullptr;
  }
  MemoryFunctionTarget& operator=(MemoryFunctionTarget&&) = default;

  bool IsNull() const final { return false; }

  Return operator()(Args... args) const final { return callable()(args...); }

  void MoveInitializeTo(void* ptr) final {
    new (ptr) MemoryFunctionTarget(std::move(*this));
  }

 private:
  Callable& callable() {
    return *std::launder(reinterpret_cast<Callable*>(address_));
  }
  const Callable& callable() const {
    return *std::launder(reinterpret_cast<const Callable*>(address_));
  }

  void* address_;
};

template <size_t kSizeBytes>
using FunctionStorage =
    std::aligned_storage_t<kSizeBytes, alignof(std::max_align_t)>;

// A FunctionTargetHolder stores an instance of a FunctionTarget implementation.
//
// The concrete implementation is initialized in an internal buffer by calling
// one of the initialization functions. After initialization, all
// implementations are accessed through the virtual FunctionTarget base.
template <size_t kSizeBytes, typename Return, typename... Args>
class FunctionTargetHolder {
 public:
  FunctionTargetHolder() = default;

  FunctionTargetHolder(const FunctionTargetHolder&) = delete;
  FunctionTargetHolder(FunctionTargetHolder&&) = delete;
  FunctionTargetHolder& operator=(const FunctionTargetHolder&) = delete;
  FunctionTargetHolder& operator=(FunctionTargetHolder&&) = delete;

  constexpr void InitializeNullTarget() {
    using NullFunctionTarget = NullFunctionTarget<Return, Args...>;
    static_assert(sizeof(NullFunctionTarget) <= kSizeBytes,
                  "NullFunctionTarget must fit within FunctionTargetHolder");
    new (&bits_) NullFunctionTarget;
  }

  // Initializes an InlineFunctionTarget with the callable, failing if it is too
  // large.
  template <typename Callable>
  void InitializeInlineTarget(Callable callable) {
    using InlineFunctionTarget =
        InlineFunctionTarget<Callable, Return, Args...>;
    static_assert(sizeof(InlineFunctionTarget) <= kSizeBytes,
                  "Inline callable must fit within FunctionTargetHolder");
    new (&bits_) InlineFunctionTarget(std::move(callable));
  }

  // Initializes a MemoryTarget that stores the callable at the provided
  // location.
  template <typename Callable>
  void InitializeMemoryTarget(Callable callable, void* storage) {
    using MemoryFunctionTarget =
        MemoryFunctionTarget<Callable, Return, Args...>;
    static_assert(sizeof(MemoryFunctionTarget) <= kSizeBytes,
                  "MemoryFunctionTarget must fit within FunctionTargetHolder");
    new (&bits_) MemoryFunctionTarget(storage, std::move(callable));
  }

  void DestructTarget() { target().~Target(); }

  // Initializes the function target within this callable from another target
  // holder's function target.
  void MoveInitializeTargetFrom(FunctionTargetHolder& other) {
    other.target().MoveInitializeTo(&bits_);
  }

  // The stored implementation is accessed by punning to the virtual base class.
  using Target = FunctionTarget<Return, Args...>;
  Target& target() { return *std::launder(reinterpret_cast<Target*>(&bits_)); }
  const Target& target() const {
    return *std::launder(reinterpret_cast<const Target*>(&bits_));
  }

 private:
  // Storage for an implementation of the FunctionTarget interface.
  FunctionStorage<kSizeBytes> bits_;
};

template <typename Return, typename... Args>
class Function;

template <typename Return, typename... Args>
class Function<Return(Args...)> {
 public:
  constexpr Function() { holder_.InitializeNullTarget(); }
  constexpr Function(std::nullptr_t) : Function() {}

  template <typename Callable>
  Function(Callable callable) {
    if (IsNull(callable)) {
      holder_.InitializeNullTarget();
    } else {
      holder_.InitializeInlineTarget(std::move(callable));
    }
  }

  Function(Function&& other) {
    holder_.MoveInitializeTargetFrom(other.holder_);
    other.holder_.InitializeNullTarget();
  }

  Function& operator=(Function&& other) {
    holder_.DestructTarget();
    holder_.MoveInitializeTargetFrom(other.holder_);
    other.holder_.InitializeNullTarget();
    return *this;
  }

  Function& operator=(std::nullptr_t) {
    holder_.DestructTarget();
    holder_.InitializeNullTarget();
    return *this;
  }

  template <typename Callable>
  Function& operator=(Callable callable) {
    holder_.DestructTarget();
    InitializeTarget(std::move(callable));
    return *this;
  }

  ~Function() { holder_.DestructTarget(); }

  Return operator()(Args... args) const { return holder_.target()(args...); };

  explicit operator bool() const { return !holder_.target().IsNull(); }

 private:
  // TODO(frolv): This is temporarily private while the API is worked out.
  template <typename Callable, size_t kSizeBytes>
  Function(Callable&& callable, FunctionStorage<kSizeBytes>& storage)
      : Function(callable, &storage) {
    static_assert(sizeof(Callable) <= kSizeBytes,
                  "pw::Function callable does not fit into provided storage");
  }

  // Constructs a function that stores its callable at the provided location.
  // Public constructors wrapping this must ensure that the memory region is
  // capable of storing the callable in terms of both size and alignment.
  template <typename Callable>
  Function(Callable&& callable, void* storage) {
    if (IsNull(callable)) {
      holder_.InitializeNullTarget();
    } else {
      holder_.InitializeMemoryTarget(std::move(callable), storage);
    }
  }

  FunctionTargetHolder<config::kInlineCallableSize, Return, Args...> holder_;
};

}  // namespace pw::function_internal
