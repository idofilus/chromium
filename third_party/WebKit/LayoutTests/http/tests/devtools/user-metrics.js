// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests list of user metrics codes and invocations.\n`);
  await TestRunner.loadModule('performance_test_runner');
  await TestRunner.loadModule('profiler_test_runner');
  await TestRunner.loadModule('audits_test_runner');

  InspectorFrontendHost.recordEnumeratedHistogram = function(name, code) {
    if (name === 'DevTools.ActionTaken')
      TestRunner.addResult('Action taken: ' + nameOf(Host.UserMetrics.Action, code));
    else if (name === 'DevTools.PanelShown')
      TestRunner.addResult('Panel shown: ' + nameOf(Host.UserMetrics._PanelCodes, code));
  };

  function nameOf(object, code) {
    for (var name in object) {
      if (object[name] === code)
        return name;
    }
    return null;
  }

  TestRunner.addResult('recordActionTaken:');
  TestRunner.dump(Host.UserMetrics.Action);
  Host.userMetrics.actionTaken(Host.UserMetrics.Action.WindowDocked);
  Host.userMetrics.actionTaken(Host.UserMetrics.Action.WindowUndocked);

  TestRunner.addResult('\nrecordPanelShown:');
  TestRunner.dump(Host.UserMetrics._PanelCodes);
  UI.viewManager.showView('js_profiler');
  UI.viewManager.showView('timeline');
  UI.viewManager.showView('audits');

  TestRunner.completeTest();
})();
