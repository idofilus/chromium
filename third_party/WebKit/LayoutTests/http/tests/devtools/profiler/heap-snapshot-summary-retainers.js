// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests retainers view.
    - Number of retainers of an A object must be 2 (A itself and B).
    - When an object has just one retainer it must be expanded automatically until
      there's an object having two or more retainers.
    - Test the expansion of a long retainment chain is limited by a certain level.\n`);
  await TestRunner.loadModule('heap_snapshot_test_runner');
  await TestRunner.showPanel('heap_profiler');

  var instanceCount = 25;
  function createHeapSnapshot() {
    return HeapSnapshotTestRunner.createHeapSnapshot(instanceCount);
  }

  HeapSnapshotTestRunner.runHeapSnapshotTestSuite([
    function testRetainersView(next) {
      HeapSnapshotTestRunner.takeAndOpenSnapshot(createHeapSnapshot, step1);

      function step1() {
        HeapSnapshotTestRunner.switchToView('Summary', step2);
      }

      function step2() {
        var row = HeapSnapshotTestRunner.findRow('A');
        TestRunner.assertEquals(true, !!row, '"A" row');
        HeapSnapshotTestRunner.expandRow(row, step3);
      }

      function step3(row) {
        var count = row.data['count'];
        TestRunner.assertEquals(instanceCount.toString(), count);
        HeapSnapshotTestRunner.clickRowAndGetRetainers(row.children[0], step4);
      }

      function step4(retainersRoot) {
        var rowsShown = HeapSnapshotTestRunner.countDataRows(retainersRoot);
        TestRunner.assertEquals(2, rowsShown, 'retaining objects');
        setTimeout(next, 0);
      }
    },

    function testRetainersAutoExpandSingleRetainer(next) {
      function createHeapSnapshot() {
        // Mocking results of running the following code:
        //
        // function L1(x) { this.x = x; }
        // function L2(y) { this.y = y; }
        // function L3() { }
        // var l1 = new L1(new L2(new L3()));
        // var root = { l1a: l1, l1b: l1 };
        // l1 = null;

        var sizeOfL3 = 1000000;
        var builder = new HeapSnapshotTestRunner.HeapSnapshotBuilder();
        var rootNode = builder.rootNode;

        var gcRootsNode = new HeapSnapshotTestRunner.HeapNode('(GC roots)');
        rootNode.linkNode(gcRootsNode, HeapSnapshotTestRunner.HeapEdge.Type.element);

        var windowNode = new HeapSnapshotTestRunner.HeapNode('Window', 20);
        rootNode.linkNode(windowNode, HeapSnapshotTestRunner.HeapEdge.Type.shortcut);
        gcRootsNode.linkNode(windowNode, HeapSnapshotTestRunner.HeapEdge.Type.element);

        var l3Node = new HeapSnapshotTestRunner.HeapNode('L3', sizeOfL3);
        var l2Node = new HeapSnapshotTestRunner.HeapNode('L2', 32);
        var l1Node = new HeapSnapshotTestRunner.HeapNode('L1', 32);
        var rootNode = new HeapSnapshotTestRunner.HeapNode('Object', 32);
        windowNode.linkNode(rootNode, HeapSnapshotTestRunner.HeapEdge.Type.property, 'root');
        rootNode.linkNode(l1Node, HeapSnapshotTestRunner.HeapEdge.Type.property, 'l1a');
        rootNode.linkNode(l1Node, HeapSnapshotTestRunner.HeapEdge.Type.property, 'l1b');
        l1Node.linkNode(l2Node, HeapSnapshotTestRunner.HeapEdge.Type.property, 'x');
        l2Node.linkNode(l3Node, HeapSnapshotTestRunner.HeapEdge.Type.property, 'y');
        return builder.generateSnapshot();
      }

      HeapSnapshotTestRunner.takeAndOpenSnapshot(createHeapSnapshot, step1);

      function step1() {
        HeapSnapshotTestRunner.switchToView('Summary', step2);
      }

      function step2() {
        var row = HeapSnapshotTestRunner.findRow('L3');
        TestRunner.assertEquals(true, !!row, '"L3" row');
        HeapSnapshotTestRunner.expandRow(row, step3);
      }

      function step3(row) {
        var count = row.data['count'];
        TestRunner.assertEquals('1', count);
        HeapSnapshotTestRunner.clickRowAndGetRetainers(row.children[0], step4);
      }

      function step4(retainersRoot) {
        retainersRoot.dataGrid.addEventListener(
            Profiler.HeapSnapshotRetainmentDataGrid.Events.ExpandRetainersComplete, step5.bind(this, retainersRoot));
      }

      function step5(retainersRoot) {
        var l3 = retainersRoot;
        TestRunner.assertEquals(1, l3.children.length, 'One retainer of L3');
        var l2 = l3.children[0];
        TestRunner.assertEquals('y', l2._referenceName);
        TestRunner.assertEquals(1, l2.children.length, 'One retainer of L2');
        var l1 = l2.children[0];
        TestRunner.assertEquals('x', l1._referenceName);
        TestRunner.assertEquals(2, l1.children.length, 'Two retainers of L1');
        var l1retainers = [l1.children[0]._referenceName, l1.children[1]._referenceName];
        l1retainers.sort();
        TestRunner.assertEquals('l1a', l1retainers[0]);
        TestRunner.assertEquals('l1b', l1retainers[1]);
        setTimeout(next, 0);
      }
    },

    function testRetainersAutoExpandSingleRetainerLimit(next) {
      function createHeapSnapshot() {
        // Mocking results of running the following code:
        //
        // function Tail() {}
        // var head = new Tail();
        // for (var i = 0; i < 1000; ++i)
        //   head = { next: head };

        var builder = new HeapSnapshotTestRunner.HeapSnapshotBuilder();
        var rootNode = builder.rootNode;

        var gcRootsNode = new HeapSnapshotTestRunner.HeapNode('(GC roots)');
        rootNode.linkNode(gcRootsNode, HeapSnapshotTestRunner.HeapEdge.Type.element);

        var windowNode = new HeapSnapshotTestRunner.HeapNode('Window', 20);
        rootNode.linkNode(windowNode, HeapSnapshotTestRunner.HeapEdge.Type.shortcut);
        gcRootsNode.linkNode(windowNode, HeapSnapshotTestRunner.HeapEdge.Type.element);

        var headNode = new HeapSnapshotTestRunner.HeapNode('Object', 32);
        windowNode.linkNode(headNode, HeapSnapshotTestRunner.HeapEdge.Type.property, 'head');
        for (var i = 1; i < 1000; ++i) {
          var nextNode = new HeapSnapshotTestRunner.HeapNode('Object', 32);
          headNode.linkNode(nextNode, HeapSnapshotTestRunner.HeapEdge.Type.property, 'next');
          headNode = nextNode;
        }
        var tailNode = new HeapSnapshotTestRunner.HeapNode('Tail', 32);
        headNode.linkNode(tailNode, HeapSnapshotTestRunner.HeapEdge.Type.property, 'next');
        return builder.generateSnapshot();
      }

      HeapSnapshotTestRunner.takeAndOpenSnapshot(createHeapSnapshot, step1);

      function step1() {
        HeapSnapshotTestRunner.switchToView('Summary', step2);
      }

      function step2() {
        var row = HeapSnapshotTestRunner.findRow('Tail');
        TestRunner.assertEquals(true, !!row, '"Tail" row');
        HeapSnapshotTestRunner.expandRow(row, step3);
      }

      function step3(row) {
        var count = row.data['count'];
        TestRunner.assertEquals('1', count);
        HeapSnapshotTestRunner.clickRowAndGetRetainers(row.children[0], step4);
      }

      function step4(retainersRoot) {
        retainersRoot.dataGrid.addEventListener(
            Profiler.HeapSnapshotRetainmentDataGrid.Events.ExpandRetainersComplete, step5.bind(this, retainersRoot));
      }

      function step5(retainersRoot) {
        var rowsShown = HeapSnapshotTestRunner.countDataRows(retainersRoot);
        TestRunner.assertEquals(20, rowsShown, 'retaining objects');
        setTimeout(next, 0);
      }
    }
  ]);
})();
