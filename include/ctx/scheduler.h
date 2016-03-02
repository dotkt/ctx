#pragma once

#include <memory>

#include "boost/asio/io_service.hpp"

#include "ctx/stack_manager.h"
#include "ctx/future.h"

namespace ctx {

struct operation;

struct scheduler {
  scheduler() = default;
  scheduler(scheduler const&) = delete;
  scheduler& operator=(scheduler const&) = delete;

  template <typename Fn>
  auto post(Fn fn) {
    using return_t = decltype(fn());
    auto f = std::make_shared<future<return_t>>();
    enqueue(std::function<void()>([f, fn]() {
      try {
        f->set(fn());
      } catch (...) {
        // TODO dont jump in catch block!
        f->set(std::current_exception());
      }
    }));
    return f;
  }

  template <typename Fn>
  auto post_void(Fn fn) {
    auto f = std::make_shared<future<void>>();
    enqueue(std::function<void()>([f, fn]() {
      try {
        fn();
        f->set();
      } catch (...) {
        // TODO dont jump in catch block!
        f->set(std::current_exception());
      }
    }));
    return f;
  }

  void enqueue(std::function<void()> fn);
  void enqueue(std::shared_ptr<operation> op);

  boost::asio::io_service ios_;
  stack_manager stack_manager_;
};

}  // namespace ctx