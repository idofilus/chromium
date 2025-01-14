// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/public/util/compositor_controller.h"

#include "base/base64.h"
#include "base/bind.h"
#include "base/json/json_writer.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "base/test/test_simple_task_runner.h"
#include "headless/public/internal/headless_devtools_client_impl.h"
#include "headless/public/util/virtual_time_controller.h"
#include "headless/public/util/testing/mock_devtools_agent_host.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace headless {

namespace {
static constexpr int kAnimationFrameIntervalMs = 16;
static constexpr base::TimeDelta kWaitForCompositorReadyFrameDelay =
    base::TimeDelta::FromMilliseconds(20);
} // namespace

using testing::Return;
using testing::_;

class TestVirtualTimeController : public VirtualTimeController {
 public:
  TestVirtualTimeController(HeadlessDevToolsClient* devtools_client)
      : VirtualTimeController(devtools_client) {}
  ~TestVirtualTimeController() override {}

  MOCK_METHOD4(GrantVirtualTimeBudget,
               void(emulation::VirtualTimePolicy policy,
                    int budget_ms,
                    const base::Callback<void()>& set_up_complete_callback,
                    const base::Callback<void()>& budget_expired_callback));
  MOCK_METHOD2(ScheduleRepeatingTask,
               void(RepeatingTask* task, int interval_ms));
  MOCK_METHOD1(CancelRepeatingTask, void(RepeatingTask* task));
};

class CompositorControllerTest : public ::testing::Test {
 protected:
  CompositorControllerTest() {
    task_runner_ = base::MakeRefCounted<base::TestSimpleTaskRunner>();
    client_.SetTaskRunnerForTests(task_runner_);
    mock_host_ = base::MakeRefCounted<MockDevToolsAgentHost>();

    EXPECT_CALL(*mock_host_, IsAttached()).WillOnce(Return(false));
    EXPECT_CALL(*mock_host_, AttachClient(&client_));
    client_.AttachToHost(mock_host_.get());
    virtual_time_controller_ =
        base::MakeUnique<TestVirtualTimeController>(&client_);
    EXPECT_CALL(*virtual_time_controller_,
                ScheduleRepeatingTask(_, kAnimationFrameIntervalMs))
        .WillOnce(testing::SaveArg<0>(&task_));
    ExpectHeadlessExperimentalEnable();
    controller_ = base::MakeUnique<CompositorController>(
        task_runner_, &client_, virtual_time_controller_.get(),
        kAnimationFrameIntervalMs, kWaitForCompositorReadyFrameDelay);
    EXPECT_NE(nullptr, task_);
  }

  ~CompositorControllerTest() override {
    EXPECT_CALL(*virtual_time_controller_, CancelRepeatingTask(_));
  }

  void ExpectHeadlessExperimentalEnable() {
    last_command_id_ += 2;
    EXPECT_CALL(*mock_host_,
                DispatchProtocolMessage(
                    &client_,
                    base::StringPrintf(
                        "{\"id\":%d,\"method\":\"HeadlessExperimental.enable\","
                        "\"params\":{}}",
                        last_command_id_)))
        .WillOnce(Return(true));
  }

  void ExpectBeginFrame(const std::string& params = "") {
    last_command_id_ += 2;
    EXPECT_CALL(
        *mock_host_,
        DispatchProtocolMessage(
            &client_,
            base::StringPrintf(
                "{\"id\":%d,\"method\":\"HeadlessExperimental.beginFrame\","
                "\"params\":{%s}}",
                last_command_id_, params.c_str())))
        .WillOnce(Return(true));
  }

  void SendBeginFrameReply(bool has_damage,
                           bool main_frame_content_updated,
                           const std::string& screenshot_data) {
    auto result = headless_experimental::BeginFrameResult::Builder()
                      .SetHasDamage(has_damage)
                      .SetMainFrameContentUpdated(main_frame_content_updated)
                      .Build();
    if (screenshot_data.length())
      result->SetScreenshotData(screenshot_data);
    std::string result_json;
    auto result_value = result->Serialize();
    base::JSONWriter::Write(*result_value, &result_json);

    client_.DispatchProtocolMessage(
        mock_host_.get(),
        base::StringPrintf("{\"id\":%d,\"result\":%s}", last_command_id_,
                           result_json.c_str()));
  }

  void SendNeedsBeginFramesEvent(bool needs_begin_frames) {
    client_.DispatchProtocolMessage(
        mock_host_.get(),
        base::StringPrintf("{\"method\":\"HeadlessExperimental."
                           "needsBeginFramesChanged\",\"params\":{"
                           "\"needsBeginFrames\":%s}}",
                           needs_begin_frames ? "true" : "false"));
    // Events are dispatched asynchronously.
    task_runner_->RunPendingTasks();
  }

  void SendMainFrameReadyForScreenshotsEvent() {
    client_.DispatchProtocolMessage(mock_host_.get(),
                                    "{\"method\":\"HeadlessExperimental."
                                    "mainFrameReadyForScreenshots\",\"params\":"
                                    "{}}");
    // Events are dispatched asynchronously.
    task_runner_->RunPendingTasks();
  }

  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  scoped_refptr<MockDevToolsAgentHost> mock_host_;
  HeadlessDevToolsClientImpl client_;
  std::unique_ptr<TestVirtualTimeController> virtual_time_controller_;
  std::unique_ptr<CompositorController> controller_;
  int last_command_id_ = -2;
  TestVirtualTimeController::RepeatingTask* task_ = nullptr;
};

TEST_F(CompositorControllerTest, WaitForCompositorReady) {
  // Shouldn't send any commands yet as no needsBeginFrames event was sent yet.
  bool ready = false;
  controller_->WaitForCompositorReady(
      base::Bind([](bool* ready) { *ready = true; }, &ready));
  EXPECT_FALSE(ready);

  // Sends BeginFrames with delay while they are needed.
  SendNeedsBeginFramesEvent(true);
  EXPECT_TRUE(task_runner_->HasPendingTask());
  ExpectBeginFrame();
  task_runner_->RunPendingTasks();

  SendBeginFrameReply(true, false, std::string());

  EXPECT_TRUE(task_runner_->HasPendingTask());
  EXPECT_EQ(kWaitForCompositorReadyFrameDelay,
            task_runner_->NextPendingTaskDelay());
  ExpectBeginFrame();
  task_runner_->RunPendingTasks();

  EXPECT_FALSE(task_runner_->HasPendingTask());

  SendBeginFrameReply(false, false, std::string());

  EXPECT_TRUE(task_runner_->HasPendingTask());
  EXPECT_EQ(kWaitForCompositorReadyFrameDelay,
            task_runner_->NextPendingTaskDelay());
  ExpectBeginFrame();
  task_runner_->RunPendingTasks();

  // No new BeginFrames are scheduled when BeginFrames are not needed.
  SendNeedsBeginFramesEvent(false);
  EXPECT_FALSE(task_runner_->HasPendingTask());

  SendBeginFrameReply(false, false, std::string());
  EXPECT_FALSE(task_runner_->HasPendingTask());

  // Restarts sending BeginFrames when they are needed again.
  SendNeedsBeginFramesEvent(true);
  EXPECT_TRUE(task_runner_->HasPendingTask());
  ExpectBeginFrame();
  task_runner_->RunPendingTasks();

  // Stops sending BeginFrames when main frame becomes ready.
  SendMainFrameReadyForScreenshotsEvent();
  EXPECT_FALSE(task_runner_->HasPendingTask());

  EXPECT_FALSE(ready);
  SendBeginFrameReply(true, true, std::string());
  EXPECT_FALSE(task_runner_->HasPendingTask());
  EXPECT_TRUE(ready);
}

TEST_F(CompositorControllerTest, CaptureScreenshot) {
  SendMainFrameReadyForScreenshotsEvent();
  EXPECT_FALSE(task_runner_->HasPendingTask());

  bool done = false;
  controller_->CaptureScreenshot(
      headless_experimental::ScreenshotParamsFormat::PNG, 100,
      base::Bind(
          [](bool* done, const std::string& screenshot_data) {
            *done = true;
            EXPECT_EQ("test", screenshot_data);
          },
          &done));

  EXPECT_TRUE(task_runner_->HasPendingTask());
  ExpectBeginFrame("\"screenshot\":{\"format\":\"png\",\"quality\":100}");
  task_runner_->RunPendingTasks();

  std::string base64;
  base::Base64Encode("test", &base64);
  SendBeginFrameReply(true, true, base64);
  EXPECT_FALSE(task_runner_->HasPendingTask());
  EXPECT_TRUE(done);
}

TEST_F(CompositorControllerTest, WaitForMainFrameContentUpdate) {
  bool updated = false;
  controller_->WaitForMainFrameContentUpdate(
      base::Bind([](bool* updated) { *updated = true; }, &updated));
  EXPECT_FALSE(updated);

  // Sends BeginFrames while they are needed.
  SendNeedsBeginFramesEvent(true);
  EXPECT_TRUE(task_runner_->HasPendingTask());
  ExpectBeginFrame();
  task_runner_->RunPendingTasks();

  SendBeginFrameReply(true, false, std::string());

  EXPECT_TRUE(task_runner_->HasPendingTask());
  EXPECT_EQ(base::TimeDelta(), task_runner_->NextPendingTaskDelay());
  ExpectBeginFrame();
  task_runner_->RunPendingTasks();

  EXPECT_FALSE(task_runner_->HasPendingTask());

  SendBeginFrameReply(false, false, std::string());

  EXPECT_TRUE(task_runner_->HasPendingTask());
  EXPECT_EQ(base::TimeDelta(), task_runner_->NextPendingTaskDelay());
  ExpectBeginFrame();
  task_runner_->RunPendingTasks();

  // No new BeginFrames are scheduled when BeginFrames are not needed.
  SendNeedsBeginFramesEvent(false);
  EXPECT_FALSE(task_runner_->HasPendingTask());

  SendBeginFrameReply(false, false, std::string());
  EXPECT_FALSE(task_runner_->HasPendingTask());

  // Restarts sending BeginFrames when they are needed again.
  SendNeedsBeginFramesEvent(true);
  EXPECT_TRUE(task_runner_->HasPendingTask());
  ExpectBeginFrame();
  task_runner_->RunPendingTasks();

  // Stops sending BeginFrames when an main frame update is included.
  SendMainFrameReadyForScreenshotsEvent();
  EXPECT_FALSE(task_runner_->HasPendingTask());

  EXPECT_FALSE(updated);
  SendBeginFrameReply(true, true, std::string());
  EXPECT_FALSE(task_runner_->HasPendingTask());
  EXPECT_TRUE(updated);
}

TEST_F(CompositorControllerTest, SendsAnimationFrames) {
  bool can_continue = false;
  auto continue_callback = base::Bind(
      [](bool* can_continue) { *can_continue = true; }, &can_continue);

  // Task doesn't block virtual time request.
  task_->BudgetRequested(base::TimeDelta(), 100, continue_callback);
  EXPECT_TRUE(can_continue);
  can_continue = false;

  // Doesn't send BeginFrames before virtual time started.
  SendNeedsBeginFramesEvent(true);
  EXPECT_FALSE(task_runner_->HasPendingTask());

  // Sends a BeginFrame after interval elapsed.
  task_->IntervalElapsed(
      base::TimeDelta::FromMilliseconds(kAnimationFrameIntervalMs),
      continue_callback);
  EXPECT_FALSE(can_continue);

  EXPECT_TRUE(task_runner_->HasPendingTask());
  ExpectBeginFrame();
  task_runner_->RunPendingTasks();

  // Lets virtual time continue after BeginFrame was completed.
  SendBeginFrameReply(false, false, std::string());
  EXPECT_FALSE(task_runner_->HasPendingTask());
  EXPECT_TRUE(can_continue);
  can_continue = false;

  // Sends another BeginFrame after next interval elapsed.
  task_->IntervalElapsed(
      base::TimeDelta::FromMilliseconds(kAnimationFrameIntervalMs),
      continue_callback);
  EXPECT_FALSE(can_continue);

  EXPECT_TRUE(task_runner_->HasPendingTask());
  ExpectBeginFrame();
  task_runner_->RunPendingTasks();

  // Lets virtual time continue after BeginFrame was completed.
  SendBeginFrameReply(false, false, std::string());
  EXPECT_FALSE(task_runner_->HasPendingTask());
  EXPECT_TRUE(can_continue);
  can_continue = false;
}

TEST_F(CompositorControllerTest, SkipsAnimationFrameForScreenshots) {
  bool can_continue = false;
  auto continue_callback = base::Bind(
      [](bool* can_continue) { *can_continue = true; }, &can_continue);

  SendMainFrameReadyForScreenshotsEvent();
  EXPECT_FALSE(task_runner_->HasPendingTask());
  SendNeedsBeginFramesEvent(true);
  EXPECT_FALSE(task_runner_->HasPendingTask());

  // Doesn't send a BeginFrame after interval elapsed if a screenshot is taken
  // instead.
  task_->IntervalElapsed(
      base::TimeDelta::FromMilliseconds(kAnimationFrameIntervalMs),
      continue_callback);
  EXPECT_FALSE(can_continue);

  controller_->CaptureScreenshot(
      headless_experimental::ScreenshotParamsFormat::PNG, 100,
      base::Bind([](const std::string&) {}));

  EXPECT_TRUE(task_runner_->HasPendingTask());
  ExpectBeginFrame("\"screenshot\":{\"format\":\"png\",\"quality\":100}");
  task_runner_->RunPendingTasks();

  EXPECT_TRUE(can_continue);
  can_continue = false;
}

TEST_F(CompositorControllerTest,
       PostponesAnimationFrameWhenBudgetExpired) {
  bool can_continue = false;
  auto continue_callback = base::Bind(
      [](bool* can_continue) { *can_continue = true; }, &can_continue);

  SendNeedsBeginFramesEvent(true);
  EXPECT_FALSE(task_runner_->HasPendingTask());

  // Doesn't send a BeginFrame after interval elapsed if the budget also
  // expired.
  task_->IntervalElapsed(
      base::TimeDelta::FromMilliseconds(kAnimationFrameIntervalMs),
      continue_callback);
  EXPECT_FALSE(can_continue);

  task_->BudgetExpired(
      base::TimeDelta::FromMilliseconds(kAnimationFrameIntervalMs));
  EXPECT_TRUE(can_continue);
  can_continue = false;
  // Flush cancelled task.
  task_runner_->RunPendingTasks();

  // Sends a BeginFrame when more virtual time budget is requested.
  ExpectBeginFrame();
  task_->BudgetRequested(
      base::TimeDelta::FromMilliseconds(kAnimationFrameIntervalMs), 100,
      continue_callback);
  EXPECT_FALSE(can_continue);

  SendBeginFrameReply(false, false, std::string());
  EXPECT_FALSE(task_runner_->HasPendingTask());
  EXPECT_TRUE(can_continue);
  can_continue = false;
}

TEST_F(CompositorControllerTest,
       SkipsAnimationFrameWhenBudgetExpiredAndScreenshotWasTaken) {
  bool can_continue = false;
  auto continue_callback = base::Bind(
      [](bool* can_continue) { *can_continue = true; }, &can_continue);

  SendMainFrameReadyForScreenshotsEvent();
  EXPECT_FALSE(task_runner_->HasPendingTask());
  SendNeedsBeginFramesEvent(true);
  EXPECT_FALSE(task_runner_->HasPendingTask());

  // Doesn't send a BeginFrame after interval elapsed if the budget also
  // expired.
  task_->IntervalElapsed(
      base::TimeDelta::FromMilliseconds(kAnimationFrameIntervalMs),
      continue_callback);
  EXPECT_FALSE(can_continue);

  task_->BudgetExpired(
      base::TimeDelta::FromMilliseconds(kAnimationFrameIntervalMs));
  EXPECT_TRUE(can_continue);
  can_continue = false;
  // Flush cancelled task.
  task_runner_->RunPendingTasks();

  controller_->CaptureScreenshot(
      headless_experimental::ScreenshotParamsFormat::PNG, 100,
      base::Bind([](const std::string&) {}));

  EXPECT_TRUE(task_runner_->HasPendingTask());
  ExpectBeginFrame("\"screenshot\":{\"format\":\"png\",\"quality\":100}");
  task_runner_->RunPendingTasks();

  // Sends a BeginFrame when more virtual time budget is requested.
  task_->BudgetRequested(
      base::TimeDelta::FromMilliseconds(kAnimationFrameIntervalMs), 100,
      continue_callback);
  EXPECT_TRUE(can_continue);
  can_continue = false;
}

TEST_F(CompositorControllerTest, WaitUntilIdle) {
  bool idle = false;
  auto idle_callback = base::Bind([](bool* idle) { *idle = true; }, &idle);

  SendNeedsBeginFramesEvent(true);
  EXPECT_FALSE(task_runner_->HasPendingTask());

  // WaitUntilIdle executes callback immediately if no BeginFrame is active.
  controller_->WaitUntilIdle(idle_callback);
  EXPECT_TRUE(idle);
  idle = false;

  // Send a BeginFrame.
  task_->IntervalElapsed(
      base::TimeDelta::FromMilliseconds(kAnimationFrameIntervalMs),
      base::Bind([]() {}));

  EXPECT_TRUE(task_runner_->HasPendingTask());
  ExpectBeginFrame();
  task_runner_->RunPendingTasks();

  // WaitUntilIdle only executes callback after BeginFrame was completed.
  controller_->WaitUntilIdle(idle_callback);
  EXPECT_FALSE(idle);

  SendBeginFrameReply(false, false, std::string());
  EXPECT_FALSE(task_runner_->HasPendingTask());
  EXPECT_TRUE(idle);
  idle = false;
}

}  // namespace headless
