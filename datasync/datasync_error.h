// Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DATASYNC_DATASYNC_ERROR_H_
#define DATASYNC_DATASYNC_ERROR_H_

#include <cassert>
#include <functional>
#include <string>

namespace datasync {

class Error {
 public:
  Error() { }
  Error(const std::string& name, const std::string& message)
      : name_(name),
        message_(message) {}
  const std::string& message() const { return message_; }
  const std::string& name() const { return name_; }

 private:
  std::string name_;
  std::string message_;
};

/**
 * Abstraction for returning result or error from function as one piece
 *
 * This type allows you to pass two callback to this object which will
 * be called respectively on Success or Failure.
 *
 * Code example:
 *
 * ResultOrError<std::string> func(bool ok) {
 *   return ok ? "string" : Error("string", "string");
 * }
 *
 * func(x).Success([]() {
 *   // success of calculations...
 * }).Failure([]() {
 *   // failure of calculations...
 * });
 *
 */
template<class Result> class ResultOrError {
 public:
  typedef std::function<void(Result)> SuccessFunction;
  typedef std::function<void(const Error&)> HandlerFunction;

  ResultOrError(const Error& ex)  // NOLINT
      : error_(ex),
        is_error_(true) {
  }

  ResultOrError(Result r)  // NOLINT
      : result_(r),
        is_error_(false) {
  }

  const ResultOrError& Failure(const HandlerFunction& f) const {
    if (is_error()) {
      f(error_);
    }
    return *this;
  }

  const ResultOrError& Success(const SuccessFunction& f) const {
    if (!is_error()) {
      f(result_);
    }
    return *this;
  }

 private:
  bool is_error() const {
    return is_error_;
  }

  Error error_;
  Result result_;
  bool is_error_;
};

template<> class ResultOrError<void> {
 public:
  typedef std::function<void()> SuccessFunction;
  typedef std::function<void(const Error&)> HandlerFunction;

  ResultOrError<void>(const Error& ex)
      : error_(ex),
        is_error_(true) {
  }

  ResultOrError<void>() : is_error_(false) { }

  const ResultOrError<void>& Failure(const HandlerFunction& f) const {
    if (is_error()) {
      f(error_);
    }
    return *this;
  }

  const ResultOrError& Success(const SuccessFunction& f) const {
    if (!is_error()) {
      f();
    }
    return *this;
  }

 private:
  bool is_error() const {
    return is_error_;
  }

  Error error_;
  bool is_error_;
};

}  // namespace datasync

#endif  // DATASYNC_DATASYNC_ERROR_H_
