// Copyright 2017 The Chromium Authors. All
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview using private properties isn't a Closure violation in tests.
 * @suppress {accessControls}
 */

ApplicationTestRunner.registerServiceWorker = function(script, scope) {
  return TestRunner.callFunctionInPageAsync('registerServiceWorker', [script, scope]);
};

ApplicationTestRunner.waitForActivated = function(scope) {
  return TestRunner.callFunctionInPageAsync('waitForActivated', [scope]);
};

ApplicationTestRunner.unregisterServiceWorker = function(scope) {
  return TestRunner.callFunctionInPageAsync('unregisterServiceWorker', [scope]);
};

ApplicationTestRunner.postToServiceWorker = function(scope, message) {
  return TestRunner.evaluateInPageAnonymously('postToServiceWorker("' + scope + '","' + message + '")');
};

ApplicationTestRunner.waitForServiceWorker = function(callback) {
  function isRightTarget(target) {
    return TestRunner.isDedicatedWorker(target) && TestRunner.isServiceWorker(target.parentTarget());
  }

  SDK.targetManager.observeTargets({
    targetAdded: function(target) {
      if (isRightTarget(target) && callback) {
        setTimeout(callback.bind(null, target), 0);
        callback = null;
      }
    },

    targetRemoved: function(target) {}
  });
};

ApplicationTestRunner.dumpServiceWorkersView = function() {
  var swView = UI.panels.resources.visibleView;

  return swView._currentWorkersView._sectionList.childTextNodes()
      .concat(swView._otherWorkersView._sectionList.childTextNodes())
      .map(function(node) {
        return node.textContent.replace(/Received.*/, 'Received').replace(/#\d+/, '#N');
      })
      .join('\n');
};

ApplicationTestRunner.deleteServiceWorkerRegistration = function(scope) {
  TestRunner.serviceWorkerManager.registrations().valuesArray().map(function(registration) {
    if (registration.scopeURL === scope)
      TestRunner.serviceWorkerManager.deleteRegistration(registration.id);
  });
};

ApplicationTestRunner.makeFetchInServiceWorker = function(scope, url, requestInitializer, callback) {
  TestRunner.callFunctionInPageAsync('makeFetchInServiceWorker', [scope, url, requestInitializer]).then(callback);
};

TestRunner.initAsync(`
  var registrations = {};

  function registerServiceWorker(script, scope) {
    return navigator.serviceWorker.register(script, {
      scope: scope
    }).then(reg => registrations[scope] = reg);
  }

  function waitForActivated(scope) {
    let reg = registrations[scope];
    if (!reg)
      return Promise.reject(new Error('The registration'));
    let worker = reg.installing || reg.waiting || reg.active;
    if (worker.state === 'activated')
      return Promise.resolve();
    if (worker.state === 'redundant')
      return Promise.reject(new Error('The worker is redundant'));
    return new Promise(resolve => {
        worker.addEventListener('statechange', () => {
            if (worker.state === 'activated')
              resolve();
          });
      });
  }

  function postToServiceWorker(scope, message) {
    registrations[scope].active.postMessage(message);
  }

  function unregisterServiceWorker(scope) {
    var registration = registrations[scope];

    if (!registration)
      return Promise.reject('ServiceWorker for ' + scope + ' is not registered');

    return registration.unregister().then(() => delete registrations[scope]);
  }

  function makeFetchInServiceWorker(scope, url, requestInitializer) {
    let script = 'resources/network-fetch-worker.js';

    return navigator.serviceWorker.register(script, {
      scope: scope
    }).then(registration => {
      let worker = registration.installing;

      return new Promise(resolve => {
        navigator.serviceWorker.onmessage = e => {
          resolve(e.data);
        };

        worker.postMessage({
          url: url,
          init: requestInitializer
        });
      });
    });
  }
`);
