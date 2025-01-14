// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `https://crbug.com/738932 Tests the snapshot view is not empty on repeatitive expand-collapse.\n`);
  await TestRunner.loadModule('heap_snapshot_test_runner');
  await TestRunner.showPanel('heap_profiler');
  await TestRunner.loadHTML(`
      <p>
      https://crbug.com/738932
      Tests the snapshot view is not empty on repeatitive expand-collapse.
      </p>
    `);

  var instanceCount = 25;
  function createHeapSnapshot() {
    return HeapSnapshotTestRunner.createHeapSnapshot(instanceCount);
  }

  HeapSnapshotTestRunner.runHeapSnapshotTestSuite([function testShowAll(next) {
    HeapSnapshotTestRunner.takeAndOpenSnapshot(createHeapSnapshot, step1);

    function step1() {
      HeapSnapshotTestRunner.switchToView('Summary', step2);
    }

    function step2() {
      HeapSnapshotTestRunner.findAndExpandRow('A', step3);
    }

    function step3(row) {
      row.collapse();
      row.expand();
      var visibleChildren = row.children.filter(c => c._element.classList.contains('revealed'));
      TestRunner.assertEquals(11, visibleChildren.length);
      next();
    }
  }]);
})();
