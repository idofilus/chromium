// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Tests that weak references are ignored when dominators are calculated and that weak references won't affect object's retained size.\n`);
  await TestRunner.loadModule('heap_snapshot_test_runner');
  await TestRunner.showPanel('heap_profiler');
  await TestRunner.loadHTML(`
      <pre></pre>
    `);

  HeapSnapshotTestRunner.runHeapSnapshotTestSuite([function testWeakReferencesDoNotAffectRetainedSize(next) {
    function createHeapSnapshot() {
      // Mocking a heap snapshot with a node that retained by weak references only.
      var builder = new HeapSnapshotTestRunner.HeapSnapshotBuilder();
      var rootNode = builder.rootNode;

      var gcRootsNode = new HeapSnapshotTestRunner.HeapNode('(GC roots)');
      rootNode.linkNode(gcRootsNode, HeapSnapshotTestRunner.HeapEdge.Type.element);

      var windowNode = new HeapSnapshotTestRunner.HeapNode('Window', 10);
      rootNode.linkNode(windowNode, HeapSnapshotTestRunner.HeapEdge.Type.shortcut);
      gcRootsNode.linkNode(windowNode, HeapSnapshotTestRunner.HeapEdge.Type.element);

      var orphanNode = new HeapSnapshotTestRunner.HeapNode('Orphan', 2000);
      windowNode.linkNode(orphanNode, HeapSnapshotTestRunner.HeapEdge.Type.weak, 'weak_ref');

      var orphanChildNode = new HeapSnapshotTestRunner.HeapNode('OrphanChild', 2000);
      orphanNode.linkNode(orphanChildNode, HeapSnapshotTestRunner.HeapEdge.Type.property, 'child');

      var aNode = new HeapSnapshotTestRunner.HeapNode('A', 300);
      windowNode.linkNode(aNode, HeapSnapshotTestRunner.HeapEdge.Type.property, 'a');

      var bNode = new HeapSnapshotTestRunner.HeapNode('B', 300);
      aNode.linkNode(bNode, HeapSnapshotTestRunner.HeapEdge.Type.property, 'b');
      orphanChildNode.linkNode(bNode, HeapSnapshotTestRunner.HeapEdge.Type.property, 'b');

      // Shortcut links should not affect retained sizes.
      rootNode.linkNode(windowNode, HeapSnapshotTestRunner.HeapEdge.Type.shortcut, 'w');
      rootNode.linkNode(aNode, HeapSnapshotTestRunner.HeapEdge.Type.shortcut, 'a');
      rootNode.linkNode(bNode, HeapSnapshotTestRunner.HeapEdge.Type.shortcut, 'b');
      rootNode.linkNode(orphanNode, HeapSnapshotTestRunner.HeapEdge.Type.shortcut, 'o');

      return builder.generateSnapshot();
    }

    TestRunner.addSniffer(Profiler.HeapSnapshotView.prototype, '_retrieveStatistics', checkStatistics);
    HeapSnapshotTestRunner.takeAndOpenSnapshot(createHeapSnapshot, step1);

    async function checkStatistics(arg, result) {
      var statistics = await result;
      TestRunner.assertEquals(4610, statistics.total);
      TestRunner.assertEquals(4610, statistics.v8heap);
      TestRunner.addResult('SUCCESS: total size is correct.');
    }

    function step1() {
      HeapSnapshotTestRunner.switchToView('Summary', step2);
    }

    function step2() {
      var row = HeapSnapshotTestRunner.findRow('A');
      TestRunner.assertEquals(true, !!row, '"A" row');
      TestRunner.assertEquals(1, row._count);
      TestRunner.assertEquals(300, row._retainedSize);
      TestRunner.assertEquals(300, row._shallowSize);


      row = HeapSnapshotTestRunner.findRow('B');
      TestRunner.assertEquals(true, !!row, '"B" row');
      TestRunner.assertEquals(1, row._count);
      TestRunner.assertEquals(300, row._retainedSize);
      TestRunner.assertEquals(300, row._shallowSize);

      row = HeapSnapshotTestRunner.findRow('Orphan');
      TestRunner.assertEquals(true, !!row, '"Orphan" row');
      TestRunner.assertEquals(1, row._count);
      TestRunner.assertEquals(4000, row._retainedSize);
      TestRunner.assertEquals(2000, row._shallowSize);


      row = HeapSnapshotTestRunner.findRow('OrphanChild');
      TestRunner.assertEquals(true, !!row, '"OrphanChild" row');
      TestRunner.assertEquals(1, row._count);
      TestRunner.assertEquals(2000, row._retainedSize);
      TestRunner.assertEquals(2000, row._shallowSize);

      TestRunner.addResult('SUCCESS: all nodes have expected retained sizes.');
      setTimeout(next, 0);
    }
  }]);
})();
