// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_TEST_VR_COMMON_TEST_SUITE_H_
#define CHROME_BROWSER_VR_TEST_VR_COMMON_TEST_SUITE_H_

#include "base/test/test_suite.h"

namespace base {
namespace test {
class ScopedTaskEnvironment;
}  // namespace test
}  // namespace base

namespace vr {

class VrCommonTestSuite : public base::TestSuite {
 public:
  VrCommonTestSuite(int argc, char** argv);
  ~VrCommonTestSuite() override;

 protected:
  void Initialize() override;
  void Shutdown() override;

 private:
  std::unique_ptr<base::test::ScopedTaskEnvironment> scoped_task_environment_;

  DISALLOW_COPY_AND_ASSIGN(VrCommonTestSuite);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_TEST_VR_COMMON_TEST_SUITE_H_
