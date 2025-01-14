// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Tests that JS object to node resolution still works even if script evals are prohibited by Content-Security-Policy. The test passes if it doesn't crash. Bug 78705.\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.showPanel('elements');
  await TestRunner.loadHTML(`
      <p>
      Tests that JS object to node resolution still works even if script evals are prohibited by Content-Security-Policy.
      The test passes if it doesn't crash.
      <a href="https://bugs.webkit.org/show_bug.cgi?id=78705">Bug 78705.</a>
      </p>
    `);

  TestRunner.evaluateInPage('document', didReceiveDocumentObject);
  async function didReceiveDocumentObject(remoteObject) {
    TestRunner.addResult('didReceiveDocumentObject');
    var nodeId = await TestRunner.DOMAgent.requestNode(remoteObject.objectId);
    TestRunner.addResult('didRequestNode error = ' + (nodeId ? 'null' : 'error'));
    TestRunner.completeTest();
  }
})();
