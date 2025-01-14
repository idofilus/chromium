// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests formatting of different types of remote objects.\n`);


  function callback(result) {
    TestRunner.addResult('date = ' + result.description.substring(0, 25));
    TestRunner.completeTest();
  }
  TestRunner.evaluateInPage('new Date(2011, 11, 7, 12, 01)', callback);
})();
