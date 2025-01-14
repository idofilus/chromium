// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests sorting in Summary view of detailed heap snapshots.\n`);
  await TestRunner.loadModule('heap_snapshot_test_runner');
  await TestRunner.showPanel('heap_profiler');

  var instanceCount = 25;
  function createHeapSnapshot() {
    return HeapSnapshotTestRunner.createHeapSnapshot(instanceCount);
  }

  HeapSnapshotTestRunner.runHeapSnapshotTestSuite([function testSorting(next) {
    HeapSnapshotTestRunner.takeAndOpenSnapshot(createHeapSnapshot, step1);

    function step1() {
      HeapSnapshotTestRunner.switchToView('Summary', step2);
    }

    var columns;
    var currentColumn;
    var currentColumnOrder;

    function step2() {
      columns = HeapSnapshotTestRunner.viewColumns();
      currentColumn = 0;
      currentColumnOrder = false;
      setTimeout(step3, 0);
    }

    function step3() {
      if (currentColumn >= columns.length) {
        setTimeout(next, 0);
        return;
      }

      HeapSnapshotTestRunner.clickColumn(columns[currentColumn], step4);
    }

    function step4(newColumnState) {
      columns[currentColumn] = newColumnState;
      var contents = HeapSnapshotTestRunner.columnContents(columns[currentColumn]);
      TestRunner.assertEquals(true, !!contents.length, 'column contents');
      var sortTypes = {object: 'text', distance: 'number', count: 'number', shallowSize: 'size', retainedSize: 'size'};
      TestRunner.assertEquals(true, !!sortTypes[columns[currentColumn].id], 'sort by id');
      HeapSnapshotTestRunner.checkArrayIsSorted(
          contents, sortTypes[columns[currentColumn].id], columns[currentColumn].sort);

      if (!currentColumnOrder)
        currentColumnOrder = true;
      else {
        currentColumnOrder = false;
        ++currentColumn;
      }
      setTimeout(step3, 0);
    }
  }]);
})();
