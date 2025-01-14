// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import android.net.Uri;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.widget.TextView;

import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.interventions.FramebustBlockMessageDelegate;
import org.chromium.chrome.browser.interventions.FramebustBlockMessageDelegateBridge;
import org.chromium.components.url_formatter.UrlFormatter;

/**
 * This InfoBar is shown to let the user know about a blocked Framebust and offer to
 * continue the redirection by tapping on a link.
 *
 * {@link FramebustBlockMessageDelegate} defines the messages shown in the infobar and
 * the target of the link.
 */
public class FramebustBlockInfoBar extends InfoBar {
    private final FramebustBlockMessageDelegate mDelegate;

    /** Whether the infobar should be shown as a mini-infobar or a classic expanded one. */
    private boolean mIsExpanded;

    @VisibleForTesting
    public FramebustBlockInfoBar(FramebustBlockMessageDelegate delegate) {
        super(delegate.getIconResourceId(), null, null);
        mDelegate = delegate;
    }

    @Override
    public void onButtonClicked(boolean isPrimaryButton) {
        assert isPrimaryButton;
        onCloseButtonClicked();
    }

    @Override
    public void createContent(InfoBarLayout layout) {
        layout.setMessage(mDelegate.getLongMessage());
        InfoBarControlLayout control = layout.addControlLayout();

        ViewGroup ellipsizerView =
                (ViewGroup) LayoutInflater.from(getContext())
                        .inflate(R.layout.infobar_control_url_ellipsizer, control, false);

        // Formatting the URL and requesting to omit the scheme might still include it for some of
        // them (e.g. file, filesystem). We split the output of the formatting to make sure we don't
        // end up duplicating it.
        String url = mDelegate.getBlockedUrl();
        String formattedUrl = UrlFormatter.formatUrlForSecurityDisplay(url, true);
        String scheme = Uri.parse(url).getScheme() + "://";

        TextView schemeView = ellipsizerView.findViewById(R.id.url_scheme);
        schemeView.setText(scheme);

        TextView urlView = ellipsizerView.findViewById(R.id.url_minus_scheme);
        urlView.setText(formattedUrl.substring(scheme.length()));

        ellipsizerView.setOnClickListener(view -> onLinkClicked());

        control.addView(ellipsizerView);
        layout.setButtons(getContext().getResources().getString(R.string.ok), null);
    }

    @Override
    protected void createCompactLayoutContent(InfoBarCompactLayout layout) {
        new InfoBarCompactLayout.MessageBuilder(layout)
                .withText(mDelegate.getShortMessage())
                .withLink(R.string.details_link, view -> onLinkClicked())
                .buildAndInsert();
    }

    @Override
    protected boolean usesCompactLayout() {
        return !mIsExpanded;
    }

    @Override
    public void onLinkClicked() {
        if (!mIsExpanded) {
            mIsExpanded = true;
            replaceView(createView());
            return;
        }

        mDelegate.onLinkTapped();
    }

    @CalledByNative
    private static FramebustBlockInfoBar create(long nativeFramebustBlockMessageDelegateBridge) {
        return new FramebustBlockInfoBar(
                new FramebustBlockMessageDelegateBridge(nativeFramebustBlockMessageDelegateBridge));
    }
}
