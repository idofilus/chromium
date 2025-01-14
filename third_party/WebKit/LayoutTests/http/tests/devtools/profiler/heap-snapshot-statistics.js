// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests Statistics view of detailed heap snapshots.\n`);
  await TestRunner.loadModule('heap_snapshot_test_runner');
  await TestRunner.showPanel('heap_profiler');

  function createHeapSnapshot() {
    var builder = new HeapSnapshotTestRunner.HeapSnapshotBuilder();
    var index = 0;
    for (type in HeapSnapshotTestRunner.HeapNode.Type) {
      if (!HeapSnapshotTestRunner.HeapNode.Type.hasOwnProperty(type))
        continue;
      if (type === HeapSnapshotTestRunner.HeapNode.Type.synthetic)
        continue;
      if (type === HeapSnapshotTestRunner.HeapNode.Type.number)
        continue;
      ++index;
      var size = index * Math.pow(10, index - 1);
      var node = new HeapSnapshotTestRunner.HeapNode(type, size, HeapSnapshotTestRunner.HeapNode.Type[type]);
      TestRunner.addResult(type + ' node size: ' + size);
      builder.rootNode.linkNode(node, HeapSnapshotTestRunner.HeapEdge.Type.internal, type + 'Link');
    }
    var jsArrayNode = new HeapSnapshotTestRunner.HeapNode('Array', 8, HeapSnapshotTestRunner.HeapNode.Type.object);
    builder.rootNode.linkNode(jsArrayNode, HeapSnapshotTestRunner.HeapEdge.Type.internal, 'JSArrayLink');
    var jsArrayContentsNode = new HeapSnapshotTestRunner.HeapNode('', 12, HeapSnapshotTestRunner.HeapNode.Type.array);
    jsArrayNode.linkNode(jsArrayContentsNode, HeapSnapshotTestRunner.HeapEdge.Type.internal, 'elements');
    var gcRootsNode =
        new HeapSnapshotTestRunner.HeapNode('(GC roots)', 0, HeapSnapshotTestRunner.HeapNode.Type.synthetic);
    builder.rootNode.linkNode(gcRootsNode, HeapSnapshotTestRunner.HeapEdge.Type.internal, '0');
    var strongRoots =
        new HeapSnapshotTestRunner.HeapNode('(Strong roots)', 0, HeapSnapshotTestRunner.HeapNode.Type.synthetic);
    gcRootsNode.linkNode(strongRoots, HeapSnapshotTestRunner.HeapEdge.Type.internal, '0');
    var systemObj =
        new HeapSnapshotTestRunner.HeapNode('SystemObject', 900000000, HeapSnapshotTestRunner.HeapNode.Type.object);
    strongRoots.linkNode(systemObj, HeapSnapshotTestRunner.HeapEdge.Type.internal, '0');
    return builder.generateSnapshot();
  }

  HeapSnapshotTestRunner.runHeapSnapshotTestSuite([function testStatistics(next) {
    TestRunner.addSniffer(Profiler.HeapSnapshotView.prototype, '_retrieveStatistics', step1);
    HeapSnapshotTestRunner.takeAndOpenSnapshot(createHeapSnapshot, () => {});

    async function step1(arg, result) {
      var statistics = await result;
      TestRunner.addResult(JSON.stringify(statistics));
      setTimeout(next, 0);
    }
  }]);
})();
