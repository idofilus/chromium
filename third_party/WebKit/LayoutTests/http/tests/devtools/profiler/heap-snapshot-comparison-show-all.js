// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Tests Comparison view of detailed heap snapshots. The "Show All" button must show all nodes.\n`);
  await TestRunner.loadModule('heap_snapshot_test_runner');
  await TestRunner.showPanel('heap_profiler');

  var instanceCount = 24;
  var firstId = 100;
  function createHeapSnapshotA() {
    return HeapSnapshotTestRunner.createHeapSnapshot(instanceCount, firstId);
  }
  function createHeapSnapshotB() {
    return HeapSnapshotTestRunner.createHeapSnapshot(instanceCount + 1, firstId + instanceCount * 2);
  }

  HeapSnapshotTestRunner.runHeapSnapshotTestSuite([function testShowAll(next) {
    HeapSnapshotTestRunner.takeAndOpenSnapshot(createHeapSnapshotA, createSnapshotB);
    function createSnapshotB() {
      HeapSnapshotTestRunner.takeAndOpenSnapshot(createHeapSnapshotB, step1);
    }

    function step1() {
      HeapSnapshotTestRunner.switchToView('Comparison', step2);
    }

    function step2() {
      var row = HeapSnapshotTestRunner.findRow('A');
      TestRunner.assertEquals(true, !!row, '"A" row');
      HeapSnapshotTestRunner.expandRow(row, step3);
    }

    var countA;
    var countB;
    function step3(row) {
      countA = row._addedCount;
      TestRunner.assertEquals(true, countA > 0, 'countA > 0');
      countB = row._removedCount;
      TestRunner.assertEquals(true, countB > 0, 'countB > 0');

      var buttonsNode = HeapSnapshotTestRunner.findButtonsNode(row);
      TestRunner.assertEquals(true, !!buttonsNode, 'buttons node (added)');
      var words = buttonsNode.showAll.textContent.split(' ');
      for (var i = 0; i < words.length; ++i) {
        var maybeNumber = parseInt(words[i], 10);
        if (!isNaN(maybeNumber))
          TestRunner.assertEquals(
              countA + countB - row._dataGrid.defaultPopulateCount(), maybeNumber, buttonsNode.showAll.textContent);
      }
      HeapSnapshotTestRunner.clickShowMoreButton('showAll', buttonsNode, step4);
    }

    function step4(row) {
      var rowsShown = HeapSnapshotTestRunner.countDataRows(row, function(node) {
        return node.data.addedCount;
      });
      TestRunner.assertEquals(countA, rowsShown, 'after showAll click 1');

      countB = row._removedCount;
      TestRunner.assertEquals(true, countB > 0, 'countB > 0');
      var buttonsNode = HeapSnapshotTestRunner.findButtonsNode(row);
      TestRunner.assertEquals(false, !!buttonsNode, 'buttons node (deleted)');

      var deletedRowsShown = HeapSnapshotTestRunner.countDataRows(row, function(node) {
        return node.data.removedCount;
      });
      TestRunner.assertEquals(countB, deletedRowsShown, 'deleted rows shown');
      setTimeout(next, 0);
    }
  }]);
})();
