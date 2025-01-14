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
      // Mocking results of running the following code:
      // root = [new Uint8Array(1000), new Uint8Array(1000), new Uint8Array(1000)]
      var builder = new HeapSnapshotTestRunner.HeapSnapshotBuilder();
      var rootNode = builder.rootNode;

      var gcRootsNode = new HeapSnapshotTestRunner.HeapNode('(GC roots)');
      rootNode.linkNode(gcRootsNode, HeapSnapshotTestRunner.HeapEdge.Type.element);

      var windowNode = new HeapSnapshotTestRunner.HeapNode('Window', 20);
      rootNode.linkNode(windowNode, HeapSnapshotTestRunner.HeapEdge.Type.shortcut);
      gcRootsNode.linkNode(windowNode, HeapSnapshotTestRunner.HeapEdge.Type.element);

      var arrayNode = new HeapSnapshotTestRunner.HeapNode('Array', 10);
      windowNode.linkNode(arrayNode, HeapSnapshotTestRunner.HeapEdge.Type.property, 'root');
      var prevBufferNode = null;
      for (var i = 0; i < 3; i++) {
        var typedArrayNode = new HeapSnapshotTestRunner.HeapNode('Uint8Array', 100);
        arrayNode.linkNode(typedArrayNode, HeapSnapshotTestRunner.HeapEdge.Type.element);

        var bufferNode = new HeapSnapshotTestRunner.HeapNode('ArrayBuffer', 1000);
        typedArrayNode.linkNode(bufferNode, HeapSnapshotTestRunner.HeapEdge.Type.internal);
        if (prevBufferNode)
          prevBufferNode.linkNode(bufferNode, HeapSnapshotTestRunner.HeapEdge.Type.weak, 'weak_next');
        prevBufferNode = bufferNode;
      }

      return builder.generateSnapshot();
    }

    HeapSnapshotTestRunner.takeAndOpenSnapshot(createHeapSnapshot, step1);

    function step1() {
      HeapSnapshotTestRunner.switchToView('Summary', step2);
    }

    function step2() {
      var row = HeapSnapshotTestRunner.findRow('Array');
      TestRunner.assertEquals(true, !!row, '"Array" row');
      HeapSnapshotTestRunner.expandRow(row, step3);
    }

    function step3(row) {
      TestRunner.assertEquals(1, row._count);
      TestRunner.assertEquals(3310, row._retainedSize);
      TestRunner.assertEquals(10, row._shallowSize);
      HeapSnapshotTestRunner.expandRow(row.children[0], step4);
    }

    function step4(arrayInstanceRow) {
      TestRunner.assertEquals(2, arrayInstanceRow._distance);
      TestRunner.assertEquals(3310, arrayInstanceRow._retainedSize);
      TestRunner.assertEquals(10, arrayInstanceRow._shallowSize);

      var children = arrayInstanceRow.children;
      TestRunner.assertEquals(3, children.length);

      for (var i = 0; i < children.length; i++) {
        TestRunner.assertEquals('Uint8Array', children[i]._name);
        TestRunner.assertEquals(100, children[i]._shallowSize);
        TestRunner.assertEquals(1100, children[i]._retainedSize);
      }
      setTimeout(next, 0);
    }
  }]);
})();
