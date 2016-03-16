#pragma once

#include "ctx/condition_variable.h"

namespace ctx {

template <typename Data>
condition_variable<Data>::condition_variable()
    : caller_(reinterpret_cast<operation<Data>*>(this_op)->shared_from_this()) {
}

template <typename Data>
template <typename Predicate>
void condition_variable<Data>::wait(std::unique_lock<std::mutex>& lock,
                                    Predicate pred) {
  while (!pred()) {
    lock.unlock();
    wait();
    lock.lock();
  }
}

template <typename Data>
void condition_variable<Data>::wait() {
  auto caller_lock = caller_.lock();
  assert(caller_lock);
  caller_lock->suspend(/* finished = */ false);
}

template <typename Data>
void condition_variable<Data>::notify() {
  auto caller_lock = caller_.lock();
  if (caller_lock) {
    caller_lock->sched_.enqueue(caller_lock);
  }
}

}  // namespace ctx