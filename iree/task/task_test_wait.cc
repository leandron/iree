// Copyright 2021 The IREE Authors
//
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <atomic>
#include <chrono>
#include <thread>

#include "iree/task/task.h"
#include "iree/task/testing/task_test.h"
#include "iree/testing/gtest.h"
#include "iree/testing/status_matchers.h"

namespace {

// NOTE: we intentionally perform most signaling to/from C++ std::threads.
// This models a real application that may be passing in handles tied to custom
// or system primitives unrelated to the task system.

class TaskWaitTest : public TaskTest {};

TEST_F(TaskWaitTest, IssueSignaled) {
  iree_event_t event;
  iree_event_initialize(/*initial_state=*/true, &event);

  iree_task_wait_t task;
  iree_task_wait_initialize(&scope_, iree_event_await(&event),
                            IREE_TIME_INFINITE_FUTURE, &task);

  IREE_ASSERT_OK(SubmitTasksAndWaitIdle(&task.header, &task.header));

  iree_event_deinitialize(&event);
}

TEST_F(TaskWaitTest, IssueUnsignaled) {
  iree_event_t event;
  iree_event_initialize(/*initial_state=*/false, &event);

  iree_task_wait_t task;
  iree_task_wait_initialize(&scope_, iree_event_await(&event),
                            IREE_TIME_INFINITE_FUTURE, &task);

  // Spin up a thread that will signal the event after we start waiting on it.
  std::atomic<bool> has_signaled = {false};
  std::thread signal_thread([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    EXPECT_FALSE(has_signaled);
    has_signaled = true;
    iree_event_set(&event);
  });

  EXPECT_FALSE(has_signaled);
  IREE_ASSERT_OK(SubmitTasksAndWaitIdle(&task.header, &task.header));
  EXPECT_TRUE(has_signaled);

  signal_thread.join();
  iree_event_deinitialize(&event);
}

// sleep

// TODO(benvanik): multi-waits: join wait a/b/c to task d.
// TODO(benvanik): multi-waits: co-issue wait a/b/c to task d/e/f.

}  // namespace
