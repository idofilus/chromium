// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'cr-toggle' is a component for showing an on/off switch. It
 * fires a 'change' event *only* when its state changes as a result of a user
 * interaction. Besides just tapping the element, its state can be changed by
 * dragging (pointerdown+pointermove) the element towards the desired direction.
 */
Polymer({
  is: 'cr-toggle',

  behaviors: [Polymer.PaperRippleBehavior],

  properties: {
    checked: {
      type: Boolean,
      value: false,
      reflectToAttribute: true,
      observer: 'checkedChanged_',
      notify: true,
    },

    disabled: {
      type: Boolean,
      value: false,
      reflectToAttribute: true,
      observer: 'disabledChanged_',
    },

    tabIndex: {
      type: Number,
      value: 0,
    },
  },

  hostAttributes: {
    'aria-disabled': 'false',
    'aria-pressed': 'false',
    role: 'button',
  },

  listeners: {
    'pointerdown': 'onPointerDown_',
    'pointerup': 'onPointerUp_',
    'tap': 'onTap_',
    'keypress': 'onKeyPress_',
    'focus': 'onFocus_',
    'blur': 'onBlur_',
  },

  /** @private {?Function} */
  boundPointerMove_: null,

  /**
   * Number of pixels required to move to consider the pointermove event as
   * intentional.
   * @type {number}
   */
  MOVE_THRESHOLD_PX: 5,

  /**
   * Whether the state of the toggle has already taken into account by
   * |pointeremove| handlers. Used in the 'tap' handler.
   * @private {boolean}
   */
  handledInPointerMove_: false,

  /** @private {number} */
  lastPointerUpTime_: 0,

  /** @override */
  attached: function() {
    let direction = this.matches(':host-context([dir=rtl]) cr-toggle') ? -1 : 1;

    this.boundPointerMove_ = (e) => {
      // Prevent unwanted text selection to occur while moving the pointer, this
      // is important.
      e.preventDefault();

      let diff = e.clientX - this.pointerDownX_;
      if (Math.abs(diff) < this.MOVE_THRESHOLD_PX)
        return;

      this.handledInPointerMove_ = true;

      let shouldToggle = (diff * direction < 0 && this.checked) ||
          (diff * direction > 0 && !this.checked);
      if (shouldToggle)
        this.toggleState_(false);
    };
  },

  /** @private */
  checkedChanged_: function() {
    this.setAttribute('aria-pressed', this.checked ? 'true' : 'false');
  },

  /** @private */
  disabledChanged_: function() {
    this.tabIndex = this.disabled ? -1 : 0;
    this.setAttribute('aria-disabled', this.disabled ? 'true' : 'false');
  },

  /** @private */
  onFocus_: function() {
    this.ensureRipple();
    this.$$('paper-ripple').holdDown = true;
  },

  /** @private */
  onBlur_: function() {
    this.ensureRipple();
    this.$$('paper-ripple').holdDown = false;
  },

  /** @private */
  onPointerUp_: function(e) {
    this.lastPointerUpTime_ = e.timeStamp;
    this.removeEventListener('pointermove', this.boundPointerMove_);
  },

  /**
   * @param {!PointerEvent} e
   * @private
   */
  onPointerDown_: function(e) {
    // Don't do anything if this was not a primary button click or touch event.
    if (e.button != 0)
      return;

    // This is necessary to have follow up pointer events fire on |this|, even
    // if they occur outside of its bounds.
    this.setPointerCapture(e.pointerId);
    this.pointerDownX_ = e.clientX;
    this.handledInPointerMove_ = false;
    this.addEventListener('pointermove', this.boundPointerMove_);
  },

  /** @private */
  onTap_: function(e) {
    // Prevent |tap| event from bubbling. It can cause parents of this elements
    // to erroneously re-toggle this control.
    e.stopPropagation();

    // User gesture has already been taken care of inside |pointermove|
    // handlers, Do nothing here.
    if (this.handledInPointerMove_)
      return;

    // If no pointermove event fired, then user just tapped on the
    // toggle button and therefore it should be toggled.
    this.toggleState_(false);
  },

  /**
   * Whether the host of this element should handle a 'tap' event it received,
   * to be used when tapping on the parent is supposed to toggle the cr-toggle.
   *
   * This is necessary to avoid a corner case when pointerdown is initiated
   * in cr-toggle, but pointerup happens outside the bounds of cr-toggle, which
   * ends up firing a 'tap' event on the parent (see context at crbug.com/689158
   * and crbug.com/768555).
   * @param {!Event} e
   * @return {boolean}
   */
  shouldIgnoreHostTap: function(e) {
    let timeStamp =
        e.detail.sourceEvent ? e.detail.sourceEvent.timeStamp : e.timeStamp;
    return timeStamp == this.lastPointerUpTime_;
  },

  /**
   * @param {boolean} fromKeyboard
   * @private
   */
  toggleState_: function(fromKeyboard) {
    this.checked = !this.checked;

    if (!fromKeyboard) {
      this.ensureRipple();
      this.$$('paper-ripple').holdDown = false;
    }

    this.fire('change', this.checked);
  },

  /**
   * @param {!KeyboardEvent} e
   * @private
   */
  onKeyPress_: function(e) {
    if (e.code == 'Space' || e.code == 'Enter') {
      e.preventDefault();
      this.toggleState_(true);
    }
  },

  // customize the element's ripple
  _createRipple: function() {
    this._rippleContainer = this.$.button;
    let ripple = Polymer.PaperRippleBehavior._createRipple();
    ripple.id = 'ink';
    ripple.setAttribute('recenters', '');
    ripple.classList.add('circle', 'toggle-ink');
    return ripple;
  },
});
