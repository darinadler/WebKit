/**
 * Copyright (C) 2023 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "InjectedBundlePageFullScreenClient.h"
#include "WebPage.h"
#include <WebCore/AppHighlight.h>
#include <WebCore/TextManipulationController.h>
#include <WebCore/UserActivity.h>

#if ENABLE(META_VIEWPORT)
#include <WebCore/ViewportConfiguration.h>
#endif

namespace WebKit {

using DynamicViewportSizeUpdateID = uint64_t;

struct WebPage::Internals {
    WTF_MAKE_STRUCT_FAST_ALLOCATED;

    WebCore::PageIdentifier identifier;

#if 0
    std::unique_ptr<WebCore::Page> page;
    Ref<WebFrame> mainFrame;

    RefPtr<WebPageGroupProxy> pageGroup;

    String userAgent;
#endif

    WebCore::IntSize viewSize;
#if 0
    std::unique_ptr<DrawingArea> drawingArea;
    DrawingAreaType drawingAreaType;

#if ENABLE(PDFKIT_PLUGIN)
    HashSet<PluginView*> pluginViews;
#endif
#endif

    HashMap<TextCheckerRequestID, RefPtr<WebCore::TextCheckingRequest>> pendingTextCheckingRequestMap;

#if 0
    bool useFixedLayout { false };
#endif

    WebCore::FloatSize defaultUnobscuredSize;
    WebCore::FloatSize minimumUnobscuredSize;
    WebCore::FloatSize maximumUnobscuredSize;

    WebCore::Color underlayColor;

#if ENABLE(PDFKIT_PLUGIN)
    HashMap<PDFPluginIdentifier, WeakPtr<PDFPlugin>> pdfPlugInsWithHUD;
#endif

#if 0
    WTF::Function<void()> selectionChangedHandler;
    bool isInRedo { false };
    bool isClosed { false };
    bool tabToLinks { false };

    bool mainFrameIsScrollable { true };

    bool alwaysShowsHorizontalScroller { false };
    bool alwaysShowsVerticalScroller { false };

    bool shouldRenderCanvasInGPUProcess { false };
    bool shouldRenderDOMInGPUProcess { false };
    bool shouldPlayMediaInGPUProcess { false };
#if ENABLE(WEBGL)
    bool shouldRenderWebGLInGPUProcess { false };
#endif
#if ENABLE(APP_BOUND_DOMAINS)
    bool needsInAppBrowserPrivacyQuirks { false };
#endif

    // The layer hosting mode.
    LayerHostingMode layerHostingMode;
#endif

#if ENABLE(APP_HIGHLIGHTS)
    WebCore::CreateNewGroupForHighlight highlightIsNewGroup { WebCore::CreateNewGroupForHighlight::No };
    WebCore::HighlightRequestOriginatedInApp highlightRequestOriginatedInApp { WebCore::HighlightRequestOriginatedInApp::No };
#endif

#if 0
#if PLATFORM(COCOA)
    bool pdfPluginEnabled { false };
    bool hasCachedWindowFrame { false };
    bool selectionFlippingEnabled { false };
#endif
#endif

#if PLATFORM(COCOA)
    WebCore::FloatRect windowFrameInScreenCoordinates;
    WebCore::FloatRect windowFrameInUnflippedScreenCoordinates;
    WebCore::FloatRect viewFrameInWindowCoordinates;
    WebCore::FloatPoint accessibilityPosition;
#endif

#if 0
#if PLATFORM(COCOA)
    RetainPtr<WKAccessibilityWebPageObject> mockAccessibilityElement;
#endif

#if ENABLE(PLATFORM_DRIVEN_TEXT_CHECKING)
    UniqueRef<TextCheckingControllerProxy> textCheckingControllerProxy;
#endif

#if PLATFORM(COCOA) || PLATFORM(GTK)
    std::unique_ptr<ViewGestureGeometryCollector> viewGestureGeometryCollector;
#endif

#if PLATFORM(COCOA)
    std::optional<double> dataDetectionReferenceDate;
#endif

#if USE(ATSPI)
    RefPtr<WebCore::AccessibilityRootAtspi> accessibilityRootObject;
#endif

#if USE(GRAPHICS_LAYER_TEXTURE_MAPPER) || USE(GRAPHICS_LAYER_WC)
    uint64_t nativeWindowHandle { 0 };
#endif

#if !PLATFORM(IOS_FAMILY)
    RefPtr<PageBanner> headerBanner;
    RefPtr<PageBanner> footerBanner;
#endif
#endif

    RunLoop::Timer setCanStartMediaTimer;
#if 0
    bool mayStartMediaWhenInWindow { false };
#endif

    HashMap<WebUndoStepID, RefPtr<WebUndoStep>> undoStepMap;

#if 0
#if ENABLE(CONTEXT_MENUS)
    std::unique_ptr<API::InjectedBundle::PageContextMenuClient> contextMenuClient;
#endif
    std::unique_ptr<API::InjectedBundle::EditorClient> editorClient;
    std::unique_ptr<API::InjectedBundle::FormClient> formClient;
    std::unique_ptr<API::InjectedBundle::PageLoaderClient> loaderClient;
    std::unique_ptr<API::InjectedBundle::ResourceLoadClient> resourceLoadClient;
    std::unique_ptr<API::InjectedBundle::PageUIClient> uiClient;
#endif
#if ENABLE(FULLSCREEN_API)
    InjectedBundlePageFullScreenClient fullScreenClient;
#endif

#if 0
    UniqueRef<FindController> findController;

    UniqueRef<WebFoundTextRangeController> foundTextRangeController;

    RefPtr<WebInspector> inspector;
    RefPtr<WebInspectorUI> inspectorUI;
    RefPtr<RemoteWebInspectorUI> remoteInspectorUI;
    std::unique_ptr<WebPageInspectorTargetController> inspectorTargetController;

#if ENABLE(VIDEO_PRESENTATION_MODE)
    RefPtr<PlaybackSessionManager> playbackSessionManager;
    RefPtr<VideoFullscreenManager> videoFullscreenManager;
#endif
#endif

#if PLATFORM(IOS_FAMILY)
#if 0
    bool allowsMediaDocumentInlinePlayback { false };
#endif
    std::optional<WebCore::SimpleRange> startingGestureRange;
#endif

#if 0
#if ENABLE(FULLSCREEN_API)
    RefPtr<WebFullScreenManager> fullScreenManager;
#endif

    RefPtr<WebPopupMenu> activePopupMenu;

#if ENABLE(CONTEXT_MENUS)
    RefPtr<WebContextMenu> contextMenu;
#endif

#if ENABLE(INPUT_TYPE_COLOR)
    WebColorChooser* activeColorChooser { nullptr };
#endif

#if ENABLE(DATALIST_ELEMENT)
    WeakPtr<WebDataListSuggestionPicker> activeDataListSuggestionPicker;
#endif

#if ENABLE(DATE_AND_TIME_INPUT_TYPES)
    WeakPtr<WebDateTimeChooser> activeDateTimeChooser;
#endif

    RefPtr<WebOpenPanelResultListener> activeOpenPanelResultListener;
    RefPtr<NotificationPermissionRequestManager> notificationPermissionRequestManager;

    Ref<WebUserContentController> userContentController;

#if ENABLE(WK_WEB_EXTENSIONS)
    RefPtr<WebExtensionControllerProxy> webExtensionController;
#endif

    UniqueRef<WebScreenOrientationManager> screenOrientationManager;

#if ENABLE(GEOLOCATION)
    UniqueRef<GeolocationPermissionRequestManager> geolocationPermissionRequestManager;
#endif

#if ENABLE(MEDIA_STREAM)
    UniqueRef<UserMediaPermissionRequestManager> userMediaPermissionRequestManager;
#endif

#if ENABLE(ENCRYPTED_MEDIA)
    UniqueRef<MediaKeySystemPermissionRequestManager> mediaKeySystemPermissionRequestManager;
#endif

    std::unique_ptr<WebCore::PrintContext> printContext;
    bool inActivePrintContextAccessScope { false };
    bool shouldEndPrintingImmediately { false };

    class PrintContextAccessScope {
    public:
        PrintContextAccessScope(WebPage& webPage)
            : webPage { webPage }
            , wasInActivePrintContextAccessScope { webPage.inActivePrintContextAccessScope }
        {
            webPage->inActivePrintContextAccessScope = true;
        }

        ~PrintContextAccessScope()
        {
            webPage->inActivePrintContextAccessScope = wasInActivePrintContextAccessScope;
            if (!wasInActivePrintContextAccessScope && webPage->shouldEndPrintingImmediately)
                webPage->endPrintingImmediately();
        }
    private:
        Ref<WebPage> webPage;
        const bool wasInActivePrintContextAccessScope;
    };

    friend class PrintContextAccessScope;

#if PLATFORM(GTK)
    std::unique_ptr<WebPrintOperationGtk> printOperation;
#endif

    SandboxExtensionTracker sandboxExtensionTracker;

    RefPtr<SandboxExtension> pendingDropSandboxExtension;
    Vector<RefPtr<SandboxExtension>> pendingDropExtensionsForFileUpload;
#endif

    PAL::HysteresisActivity pageScrolledHysteresis;
#if 0
    bool canRunBeforeUnloadConfirmPanel { false };

    bool canRunModal { false };
    bool isRunningModal { false };

#if ENABLE(DRAG_SUPPORT)
    bool isStartingDrag { false };
#endif
#endif

#if ENABLE(DRAG_SUPPORT)
    OptionSet<WebCore::DragSourceAction> allowedDragSourceActions;
#endif

#if PLATFORM(IOS_FAMILY) && ENABLE(DRAG_SUPPORT)
    HashSet<RefPtr<WebCore::HTMLImageElement>> pendingImageElementsForDropSnapshot;
    std::optional<WebCore::SimpleRange> rangeForDropSnapshot;
#endif

    WebCore::RectEdges<bool> cachedMainFramePinnedState { true, true, true, true };
#if 0
    bool canShortCircuitHorizontalWheelEvents { false };
    bool hasWheelEventHandlers { false };

    unsigned cachedPageCount { 0 };
#endif

    HashSet<WebCore::ResourceLoaderIdentifier> trackedNetworkResourceRequestIdentifiers;

    WebCore::IntSize minimumSizeForAutoLayout;
    WebCore::IntSize sizeToContentAutoSizeMaximumSize;
#if 0
    bool autoSizingShouldExpandToViewHeight { false };
#endif
    std::optional<WebCore::FloatSize> viewportSizeForCSSViewportUnits;

#if 0
    bool userIsInteracting { false };

#if HAVE(TOUCH_BAR)
    bool hasEverFocusedElementDueToUserInteractionSincePageTransition { false };
    bool requiresUserActionForEditingControlsManager { false };
    bool isTouchBarUpdateSupressedForHiddenContentEditable { false };
    bool isNeverRichlyEditableForTouchBar { false };
#endif
#endif
    OptionSet<WebCore::ActivityState> lastActivityStateChanges;

#if 0
#if HAVE(UIKIT_RESIZABLE_WINDOWS)
    bool isWindowResizingEnabled { false };
#endif

#if ENABLE(CONTEXT_MENUS)
    bool waitingForContextMenuToShow { false };
#endif

    RefPtr<WebCore::Element> focusedElement;
    RefPtr<WebCore::Element> recentlyBlurredElement;
    bool hasPendingInputContextUpdateAfterBlurringAndRefocusingElement { false };
    bool pendingThemeColorChange { false };
    bool pendingPageExtendedBackgroundColorChange { false };
    bool pendingSampledPageTopColorChange { false };

    enum class PendingEditorStateUpdateStatus : uint8_t {
        NotScheduled,
        Scheduled,
        ScheduledDuringAccessibilitySelectionChange,
    };
    PendingEditorStateUpdateStatus pendingEditorStateUpdateStatus { PendingEditorStateUpdateStatus::NotScheduled };
    bool needsEditorStateVisualDataUpdate { false };
#endif

#if ENABLE(META_VIEWPORT)
    WebCore::ViewportConfiguration viewportConfiguration;
#endif

#if 0
#if ENABLE(META_VIEWPORT)
    bool useTestingViewportConfiguration { false };
    bool forceAlwaysUserScalable { false };
#endif
#endif

#if PLATFORM(IOS_FAMILY)
    std::optional<WebCore::SimpleRange> currentWordRange;
#if 0
    RefPtr<WebCore::Node> interactionNode;
#endif
    WebCore::IntPoint lastInteractionLocation;

#if 0
    bool isShowingInputViewForFocusedElement { false };
    bool wasShowingInputViewForFocusedElementDuringLastPotentialTap { false };
    bool completingSyntheticClick { false };
    bool hasHandledSyntheticClick { false };

    enum SelectionAnchor { Start, End };
    SelectionAnchor selectionAnchor { Start };

    RefPtr<WebCore::Node> potentialTapNode;
#endif
    WebCore::FloatPoint potentialTapLocation;
#if 0
    RefPtr<WebCore::SecurityOrigin> potentialTapSecurityOrigin;

    bool hasReceivedVisibleContentRectsAfterDidCommitLoad { false };
    bool hasRestoredExposedContentRectAfterDidCommitLoad { false };
    bool scaleWasSetByUIProcess { false };
    bool userHasChangedPageScaleFactor { false };
    bool hasStablePageScaleFactor { true };
    bool isInStableState { true };
    bool shouldRevealCurrentSelectionAfterInsertion { true };
    bool screenIsBeingCaptured { false };
#endif
    MonotonicTime oldestNonStableUpdateVisibleContentRectsTimestamp;
#if 0
    Seconds estimatedLatency { 0 };
#endif
    WebCore::FloatSize screenSize;
    WebCore::FloatSize availableScreenSize;
    WebCore::FloatSize overrideScreenSize;

    std::optional<WebCore::SimpleRange> initialSelection;
    WebCore::VisibleSelection storedSelectionForAccessibility;
#if 0
    WebCore::IntDegrees deviceOrientation { 0 };
    bool keyboardIsAttached { false };
    bool canShowWhileLocked { false };
    bool inDynamicSizeUpdate { false };
#endif
    HashMap<std::pair<WebCore::IntSize, double>, WebCore::IntPoint> dynamicSizeUpdateHistory;
#if 0
    RefPtr<WebCore::Node> pendingSyntheticClickNode;
#endif
    WebCore::FloatPoint pendingSyntheticClickLocation;
    WebCore::FloatRect previousExposedContentRect;
    OptionSet<WebEventModifier> pendingSyntheticClickModifiers;
#if 0
    WebCore::PointerID pendingSyntheticClickPointerId { 0 };
#endif
    FocusedElementInformationIdentifier lastFocusedElementInformationIdentifier;
    std::optional<DynamicViewportSizeUpdateID> pendingDynamicViewportSizeUpdateID;
#if 0
    double lastTransactionPageScaleFactor { 0 };
#endif
    TransactionID lastTransactionIDWithScaleChange;

    CompletionHandler<void(InteractionInformationAtPosition&&)> pendingSynchronousPositionInformationReply;
    std::optional<std::pair<TransactionID, double>> lastLayerTreeTransactionIdAndPageScaleBeforeScalingPage;
#if 0
    bool sendAutocorrectionContextAfterFocusingElement { false };
    std::unique_ptr<WebCore::IgnoreSelectionChangeForScope> ignoreSelectionChangeScopeForDictation;
#endif
#endif // PLATFORM(IOS_FAMILY)

    WebCore::Timer layerVolatilityTimer;
#if 0
    Seconds layerVolatilityTimerInterval;
    Vector<CompletionHandler<void(bool)>> markLayersAsVolatileCompletionHandlers;
    bool isSuspendedUnderLock { false };

    HashSet<String, ASCIICaseInsensitiveHash> mimeTypesWithCustomContentProviders;
#endif
    std::optional<WebCore::Color> backgroundColor { WebCore::Color::white };
#if 0
    HashSet<unsigned> activeRenderingSuppressionTokens;
    unsigned maximumRenderingSuppressionToken { 0 };

    WebCore::ScrollPinningBehavior scrollPinningBehavior { WebCore::ScrollPinningBehavior::DoNotPin };
    std::optional<WebCore::ScrollbarOverlayStyle> scrollbarOverlayStyle;

    bool useAsyncScrolling { false };
#endif

    OptionSet<WebCore::ActivityState> activityState;

#if 0
    bool isAppNapEnabled { true };
#endif
    UserActivity userActivity { "App nap disabled for page due to user activity"_s };

#if 0
    uint64_t pendingNavigationID { 0 };
#endif
    std::optional<WebsitePoliciesData> pendingWebsitePolicies;

#if 0
    bool mainFrameProgressCompleted { false };
    bool shouldDispatchFakeMouseMoveEvents { true };
    bool isSelectingTextWhileInsertingAsynchronously { false };
    bool isChangingSelectionForAccessibility { false };

    enum class EditorStateIsContentEditable { No, Yes, Unset };
    mutable EditorStateIsContentEditable lastEditorStateWasContentEditable { EditorStateIsContentEditable::Unset };
#endif
    mutable EditorStateIdentifier lastEditorStateIdentifier;

#if PLATFORM(GTK) || PLATFORM(WPE)
    std::optional<InputMethodState> inputMethodState;
#endif

#if 0
#if USE(OS_STATE)
    WallTime loadCommitTime;
#endif

    WebCore::UserInterfaceLayoutDirection userInterfaceLayoutDirection { WebCore::UserInterfaceLayoutDirection::LTR };

    const String overrideContentSecurityPolicy;
    const std::optional<double> cpuLimit;
#endif

#if USE(WPE_RENDERER)
    UnixFileDescriptor hostFileDescriptor;
#endif

#if 0
    HashMap<String, RefPtr<WebURLSchemeHandlerProxy>> schemeToURLSchemeHandlerProxyMap;
#endif
    HashMap<WebURLSchemeHandlerIdentifier, WebURLSchemeHandlerProxy*> identifierToURLSchemeHandlerProxyMap;

#if 0
    HashMap<uint64_t, Function<void(bool granted)>> storageAccessResponseCallbackMap;
#endif

    OptionSet<LayerTreeFreezeReason> layerTreeFreezeReasons;
#if 0
    bool isSuspended { false };
    bool needsFontAttributes { false };
    bool firstFlushAfterCommit { false };
#if PLATFORM(COCOA)
    WeakPtr<WebRemoteObjectRegistry> remoteObjectRegistry;
#endif
#endif
    WebPageProxyIdentifier webPageProxyIdentifier;
    std::optional<WebCore::IntSize> pendingIntrinsicContentSize;
    WebCore::IntSize lastSentIntrinsicContentSize;
#if 0
#if HAVE(VISIBILITY_PROPAGATION_VIEW)
    std::unique_ptr<LayerHostingContext> contextForVisibilityPropagation;
#endif
#endif
#if ENABLE(TEXT_AUTOSIZING)
    WebCore::Timer textAutoSizingAdjustmentTimer;
#endif
#if ENABLE(TRACKING_PREVENTION)
    HashMap<WebCore::RegistrableDomain, WebCore::RegistrableDomain> domainsWithPageLevelStorageAccess;
    HashSet<WebCore::RegistrableDomain> loadedSubresourceDomains;
#endif

    AtomString overriddenMediaType;
#if 0
    String processDisplayName;
    WebCore::AllowsContentJavaScript allowsContentJavaScriptFromMostRecentNavigation { WebCore::AllowsContentJavaScript::Yes };

#if PLATFORM(GTK)
    WebCore::Color accentColor;
#endif

#if ENABLE(APP_BOUND_DOMAINS)
    bool limitsNavigationsToAppBoundDomains { false };
    bool navigationHasOccured { false };
#endif

    bool lastNavigationWasAppInitiated { true };

    bool canUseCredentialStorage { true };

    bool didUpdateRenderingAfterCommittingLoad { false };
    bool isStoppingLoadingDueToProcessSwap { false };
    bool skipDecidePolicyForResponseIfPossible { false };

#if ENABLE(ARKIT_INLINE_PREVIEW)
    bool useARKitForModel { false };
#endif
#if HAVE(SCENEKIT)
    bool useSceneKitForModel { false };
#endif

    bool textManipulationIncludesSubframes { false };
#endif
    std::optional<Vector<WebCore::TextManipulationController::ExclusionRule>> textManipulationExclusionRules;

#if 0
    Vector<String> corsDisablingPatterns;

    std::unique_ptr<WebCore::CachedPage> cachedPage;

#if ENABLE(IPC_TESTING_API)
    bool ipcTestingAPIEnabled { false };
    uint64_t visitedLinkTableID;
#endif

#if ENABLE(MEDIA_SESSION_COORDINATOR)
    RefPtr<WebCore::MediaSessionCoordinator> mediaSessionCoordinator;
    RefPtr<RemoteMediaSessionCoordinator> remoteMediaSessionCoordinator;
#endif

#if ENABLE(GPU_PROCESS)
    std::unique_ptr<RemoteRenderingBackendProxy> remoteRenderingBackendProxy;
#endif

#if ENABLE(IMAGE_ANALYSIS)
    Vector<std::pair<WeakPtr<WebCore::HTMLElement, WebCore::WeakPtrImplWithEventTargetData>, Vector<CompletionHandler<void(RefPtr<WebCore::Element>&&)>>>> elementsPendingTextRecognition;
#endif

#if ENABLE(WEBXR) && !USE(OPENXR)
    std::unique_ptr<PlatformXRSystemProxy> xrSystemProxy;
#endif

#if ENABLE(APP_HIGHLIGHTS)
    WebCore::HighlightVisibility appHighlightsVisible { WebCore::HighlightVisibility::Hidden };
#endif

#endif

#if ENABLE(NETWORK_CONNECTION_INTEGRITY)
    HashSet<String> lookalikeCharacterStrings;
    HashMap<WebCore::RegistrableDomain, HashSet<String>> domainScopedLookalikeCharacterStrings;
    HashMap<WebCore::RegistrableDomain, HashSet<String>> allowedLookalikeCharacterStrings;
#endif

#if ENABLE(IMAGE_ANALYSIS_ENHANCEMENTS)
    WeakHashSet<WebCore::HTMLImageElement, WebCore::WeakPtrImplWithEventTargetData> elementsToExcludeFromRemoveBackground;
#endif

    Internals(WebPage&, WebCore::PageIdentifier, WebPageCreationParameters&&);
};

} // namespace WebKit
