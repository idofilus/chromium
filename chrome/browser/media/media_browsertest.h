// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_MEDIA_BROWSERTEST_H_
#define CHROME_BROWSER_MEDIA_MEDIA_BROWSERTEST_H_

#include <string>

#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents_observer.h"
#include "media/base/test_data_util.h"

namespace content {
class TitleWatcher;
}

namespace chrome {

// Class used to automate running media related browser tests. The functions
// assume that media files are located under media/ folder known to the test
// http server.
class MediaBrowserTest : public InProcessBrowserTest,
                         public content::WebContentsObserver {
 protected:
  MediaBrowserTest();
  ~MediaBrowserTest() override;

  // Runs a html page with a list of URL query parameters.
  // If http is true, the test starts a local http test server to load the test
  // page, otherwise a local file URL is loaded inside the content shell.
  // It uses RunTest() to check for expected test output.
  void RunMediaTestPage(const std::string& html_page,
                        const base::StringPairs& query_params,
                        const std::string& expected,
                        bool http);

  // Opens a URL and waits for the document title to match either one of the
  // default strings or the expected string. Returns the matching title value.
  std::string RunTest(const GURL& gurl, const std::string& expected);

  virtual void AddWaitForTitles(content::TitleWatcher* title_watcher);

  // Fails test and sets document title to kPluginCrashed when a plugin crashes.
  // If IgnorePluginCrash(true) is called then plugin crash is ignored.
  void PluginCrashed(const base::FilePath& plugin_path,
                     base::ProcessId plugin_pid) override;

  // When called, the test will ignore any plugin crashes and not fail the test.
  void IgnorePluginCrash();

 private:
  bool ignore_plugin_crash_;
};

}  // namespace chrome

#endif  // CHROME_BROWSER_MEDIA_MEDIA_BROWSERTEST_H_
