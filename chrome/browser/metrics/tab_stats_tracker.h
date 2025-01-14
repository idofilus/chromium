// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_TAB_STATS_TRACKER_H_
#define CHROME_BROWSER_METRICS_TAB_STATS_TRACKER_H_

#include <memory>

#include "base/macros.h"
#include "base/power_monitor/power_observer.h"
#include "base/sequence_checker.h"
#include "base/timer/timer.h"
#include "chrome/browser/metrics/tab_stats_data_store.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/metrics/daily_event.h"

class PrefRegistrySimple;
class PrefService;

namespace metrics {

// Class for tracking and recording the tabs and browser windows usage.
//
// This class is meant to be used as a singleton by calling the SetInstance
// method, e.g.:
//     TabStatsTracker::SetInstance(
//         std::make_unique<TabStatsTracker>(g_browser_process->local_state()));
class TabStatsTracker : public TabStripModelObserver,
                        public chrome::BrowserListObserver,
                        public base::PowerObserver {
 public:
  // Constructor. |pref_service| must outlive this object.
  explicit TabStatsTracker(PrefService* pref_service);
  ~TabStatsTracker() override;

  // Sets the |TabStatsTracker| global instance.
  static void SetInstance(std::unique_ptr<TabStatsTracker> instance);

  // Returns the |TabStatsTracker| global instance.
  static TabStatsTracker* GetInstance();

  // Registers prefs used to track tab stats.
  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Accessors.
  const TabStatsDataStore::TabsStats& tab_stats() const;

 protected:
  // The UmaStatsReportingDelegate is responsible for delivering statistics
  // reported by the TabStatsTracker via UMA.
  class UmaStatsReportingDelegate;

  // The observer that's used by |daily_event_| to report the metrics.
  class TabStatsDailyObserver : public DailyEvent::Observer {
   public:
    // Constructor. |reporting_delegate| and |data_store| must outlive this
    // object.
    TabStatsDailyObserver(UmaStatsReportingDelegate* reporting_delegate,
                          TabStatsDataStore* data_store)
        : reporting_delegate_(reporting_delegate), data_store_(data_store) {}
    ~TabStatsDailyObserver() override {}

    // Callback called when the daily event happen.
    void OnDailyEvent() override;

   private:
    // The delegate used to report the metrics.
    UmaStatsReportingDelegate* reporting_delegate_;

    // The data store that houses the metrics.
    TabStatsDataStore* data_store_;

    DISALLOW_COPY_AND_ASSIGN(TabStatsDailyObserver);
  };

  // Accessors, exposed for unittests:
  TabStatsDataStore* tab_stats_data_store() {
    return tab_stats_data_store_.get();
  }
  base::RepeatingTimer* timer() { return &timer_; }
  DailyEvent* daily_event() { return daily_event_.get(); }
  UmaStatsReportingDelegate* reporting_delegate() {
    return reporting_delegate_.get();
  }

  // Reset the DailyEvent object to |daily_event|, for testing purposes.
  void reset_daily_event(DailyEvent* daily_event) {
    daily_event_.reset(daily_event);
  }

  // chrome::BrowserListObserver:
  void OnBrowserAdded(Browser* browser) override;
  void OnBrowserRemoved(Browser* browser) override;

  // TabStripModelObserver:
  void TabInsertedAt(TabStripModel* tab_strip_model,
                     content::WebContents* contents,
                     int index,
                     bool foreground) override;
  void TabClosingAt(TabStripModel* model,
                    content::WebContents* web_contents,
                    int index) override;

  // base::PowerObserver:
  void OnResume() override;

  // The name of the histogram used to report that the daily event happened.
  static const char kTabStatsDailyEventHistogramName[];

 private:
  // The delegate that reports the events.
  std::unique_ptr<UmaStatsReportingDelegate> reporting_delegate_;

  // The tab stats.
  std::unique_ptr<TabStatsDataStore> tab_stats_data_store_;

  // A daily event for collecting metrics once a day.
  std::unique_ptr<DailyEvent> daily_event_;

  // The timer used to periodically check if the daily event should be
  // triggered.
  base::RepeatingTimer timer_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(TabStatsTracker);
};

// The reporting delegate, which reports metrics via UMA.
class TabStatsTracker::UmaStatsReportingDelegate {
 public:
  // The name of the histogram that records the number of tabs total at resume
  // from sleep/hibernate.
  static const char kNumberOfTabsOnResumeHistogramName[];

  // The name of the histogram that records the maximum number of tabs opened in
  // a day.
  static const char kMaxTabsInADayHistogramName[];

  // The name of the histogram that records the maximum number of tabs opened in
  // the same window in a day.
  static const char kMaxTabsPerWindowInADayHistogramName[];

  // The name of the histogram that records the maximum number of windows
  // opened in a day.
  static const char kMaxWindowsInADayHistogramName[];

  UmaStatsReportingDelegate() {}
  ~UmaStatsReportingDelegate() {}

  // Called at resume from sleep/hibernate.
  void ReportTabCountOnResume(size_t tab_count);

  // Called once per day to report the metrics.
  void ReportDailyMetrics(const TabStatsDataStore::TabsStats& tab_stats);

 private:
  DISALLOW_COPY_AND_ASSIGN(UmaStatsReportingDelegate);
};

}  // namespace metrics

#endif  // CHROME_BROWSER_METRICS_TAB_STATS_TRACKER_H_
