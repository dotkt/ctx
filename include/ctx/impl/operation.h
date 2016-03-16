#pragma once

#include "ctx/operation.h"
#include "ctx/scheduler.h"

namespace ctx {

template <typename Data>
operation<Data>::operation(Data data, std::function<void()> fn,
                           scheduler<Data>& sched, op_id id)
    : data_(data),
      id_(std::move(id)),
      sched_(sched),
      fn_(std::move(fn)),
      running_(false),
      reschedule_(false),
      finished_(false) {}

template <typename Data>
operation<Data>::~operation() {
  sched_.stack_manager_.dealloc(stack_);
}
template <typename Data>
void operation<Data>::resume() {
  {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (finished_) {
      return;
    }
    if (running_) {
      reschedule_ = true;
      return;
    }
    running_ = true;
  }

  if (stack_.get_stack() == nullptr) {
    init();
  }

  this_op = this;
  data_.on_resume(id_);
  auto finished =
      boost::context::jump_fcontext(&main_ctx_, op_ctx_, me(), false);

  {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (reschedule_) {
      sched_.enqueue(this->shared_from_this());
      reschedule_ = false;
    }
    finished_ = finished;
    running_ = false;
  }
}

template <typename Data>
void operation<Data>::suspend(bool finished) {
  finished ? data_.on_finish(id_) : data_.on_suspend(id_, id_);
  std::shared_ptr<operation<Data>> self =
      finished ? nullptr : this->shared_from_this();
  boost::context::jump_fcontext(&op_ctx_, main_ctx_, finished, false);
}

template <typename Data>
void operation<Data>::start() {
  fn_();
  suspend(true);
}

template <typename Data>
void operation<Data>::init() {
  stack_ = sched_.stack_manager_.alloc();
  op_ctx_ = boost::context::make_fcontext(stack_.get_stack(), kStackSize,
                                          execute<Data>);
}
template <typename Data>
intptr_t operation<Data>::me() const {
  return reinterpret_cast<intptr_t>(this);
}

}  // namespace ctx