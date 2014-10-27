// Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMMON_SCOPE_EXIT_H_
#define COMMON_SCOPE_EXIT_H_

namespace common {

/**
 * The Scope Exit idiom
 *
 * Func - callable type (function, standard functor, lambda function...)
 *
 * This idiom provides way to release resources when execution exits lexical
 * scope exit.
 *
 * Template class encapsulate callable object. This object is being called
 * automatically upon exiting function, block scope. This guarantees resource
 * cleanup at any way of exit from scope - normal function return or exception.
 *
 * Example:
 *
 *   struct unit;
 *   struct unit* unit_new();
 *   void unit_free(unit*);
 *
 *   void ExampleFunctionScope() {
 *     struct unit* u = unit_new();
 *
 *     // automatic cleanup for all cases
 *     auto scope_exit = MakeScopeExit([=]() {
 *       unit_free(u);
 *     });
 *
 *     // do 1st operation with unit and possibly quit function
 *     // do 2nd operation with unit and possibly quit function
 *     // do 3rd operation with unit and possibly quit function
 *   }
 */
template<typename Func>
class ScopeExit final {
 public:
  ScopeExit(Func func) : func_(func), released_(false) { }  // NOLINT
  ~ScopeExit() noexcept(false) {
    if (!released_)
      func_();
  }

  ScopeExit(const ScopeExit&) = delete;
  ScopeExit& operator=(const ScopeExit&) = delete;
  ScopeExit(ScopeExit&& other)  // NOLINT
      : func_(std::move(other.func_)),
        released_(other.released_) {
    other.released_ = true;
  }

 private:
  Func func_;
  bool released_;
};

template <typename F>
ScopeExit<F> MakeScopeExit(F f) {
  return ScopeExit<F>(f);
}

}  // namespace common

#endif  // COMMON_SCOPE_EXIT_H_
