/*
 * Copyright (C) 2004-2017 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "config.h"
#import "Cursor.h"

#if HAVE(NSCURSOR)

#import <AppKit/NSCursor.h>
#import <objc/runtime.h>
#import <wtf/BlockObjCExceptions.h>
#import <wtf/StdLibExtras.h>

#if HAVE(HISERVICES)
#import <pal/spi/mac/HIServicesSPI.h>
#endif

@interface WebCoreCursorBundle : NSObject { }
@end

@implementation WebCoreCursorBundle
@end

namespace WebCore {

#if HAVE(HISERVICES)

static NSCursor *busyButClickableNSCursor;
static NSCursor *makeAliasNSCursor;
static NSCursor *moveNSCursor;
static NSCursor *resizeEastNSCursor;
static NSCursor *resizeEastWestNSCursor;
static NSCursor *resizeNorthNSCursor;
static NSCursor *resizeNorthSouthNSCursor;
static NSCursor *resizeNortheastNSCursor;
static NSCursor *resizeNortheastSouthwestNSCursor;
static NSCursor *resizeNorthwestNSCursor;
static NSCursor *resizeNorthwestSoutheastNSCursor;
static NSCursor *resizeSouthNSCursor;
static NSCursor *resizeSoutheastNSCursor;
static NSCursor *resizeSouthwestNSCursor;
static NSCursor *resizeWestNSCursor;

static NSCursor *cellNSCursor;
static NSCursor *helpNSCursor;
static NSCursor *zoomInNSCursor;
static NSCursor *zoomOutNSCursor;

static NSInteger WKCoreCursor_coreCursorType(id self, SEL)
{
    if (self == busyButClickableNSCursor)
        return kCoreCursorBusyButClickable;
    if (self == makeAliasNSCursor)
        return kCoreCursorMakeAlias;
    if (self == moveNSCursor)
        return kCoreCursorWindowMove;
    if (self == resizeEastNSCursor)
        return kCoreCursorWindowResizeEast;
    if (self == resizeEastWestNSCursor)
        return kCoreCursorWindowResizeEastWest;
    if (self == resizeNorthNSCursor)
        return kCoreCursorWindowResizeNorth;
    if (self == resizeNorthSouthNSCursor)
        return kCoreCursorWindowResizeNorthSouth;
    if (self == resizeNortheastNSCursor)
        return kCoreCursorWindowResizeNorthEast;
    if (self == resizeNortheastSouthwestNSCursor)
        return kCoreCursorWindowResizeNorthEastSouthWest;
    if (self == resizeNorthwestNSCursor)
        return kCoreCursorWindowResizeNorthWest;
    if (self == resizeNorthwestSoutheastNSCursor)
        return kCoreCursorWindowResizeNorthWestSouthEast;
    if (self == resizeSouthNSCursor)
        return kCoreCursorWindowResizeSouth;
    if (self == resizeSoutheastNSCursor)
        return kCoreCursorWindowResizeSouthEast;
    if (self == resizeSouthwestNSCursor)
        return kCoreCursorWindowResizeSouthWest;
    if (self == resizeWestNSCursor)
        return kCoreCursorWindowResizeWest;
    if (self == cellNSCursor)
        return kCoreCursorCell;
    if (self == helpNSCursor)
        return kCoreCursorHelp;
    if (self == zoomInNSCursor)
        return kCoreCursorZoomIn;
    if (self == zoomOutNSCursor)
        return kCoreCursorZoomOut;
    
    return NSNotFound;
}

static Class createCoreCursorClass()
{
    Class coreCursorClass = objc_allocateClassPair([NSCursor class], "WKCoreCursor", 0);
    SEL coreCursorType = NSSelectorFromString(@"_coreCursorType");
    class_addMethod(coreCursorClass, coreCursorType, (IMP)WKCoreCursor_coreCursorType, method_getTypeEncoding(class_getInstanceMethod([NSCursor class], coreCursorType)));
    objc_registerClassPair(coreCursorClass);
    return coreCursorClass;
}

static Class coreCursorClass()
{
    Class coreCursorClass = objc_lookUpClass("WKCoreCursor");
    if (!coreCursorClass)
        coreCursorClass = createCoreCursorClass();
    return coreCursorClass;
}

static NSCursor *cursor(const char *name)
{
WTF_ALLOW_UNSAFE_BUFFER_USAGE_BEGIN
    __strong NSCursor **slot = nullptr;
    
    if (!strcmp(name, "BusyButClickable"))
        slot = &busyButClickableNSCursor;
    else if (!strcmp(name, "MakeAlias"))
        slot = &makeAliasNSCursor;
    else if (!strcmp(name, "Move"))
        slot = &moveNSCursor;
    else if (!strcmp(name, "ResizeEast"))
        slot = &resizeEastNSCursor;
    else if (!strcmp(name, "ResizeEastWest"))
        slot = &resizeEastWestNSCursor;
    else if (!strcmp(name, "ResizeNorth"))
        slot = &resizeNorthNSCursor;
    else if (!strcmp(name, "ResizeNorthSouth"))
        slot = &resizeNorthSouthNSCursor;
    else if (!strcmp(name, "ResizeNortheast"))
        slot = &resizeNortheastNSCursor;
    else if (!strcmp(name, "ResizeNortheastSouthwest"))
        slot = &resizeNortheastSouthwestNSCursor;
    else if (!strcmp(name, "ResizeNorthwest"))
        slot = &resizeNorthwestNSCursor;
    else if (!strcmp(name, "ResizeNorthwestSoutheast"))
        slot = &resizeNorthwestSoutheastNSCursor;
    else if (!strcmp(name, "ResizeSouth"))
        slot = &resizeSouthNSCursor;
    else if (!strcmp(name, "ResizeSoutheast"))
        slot = &resizeSoutheastNSCursor;
    else if (!strcmp(name, "ResizeSouthwest"))
        slot = &resizeSouthwestNSCursor;
    else if (!strcmp(name, "ResizeWest"))
        slot = &resizeWestNSCursor;
    else if (!strcmp(name, "Cell"))
        slot = &cellNSCursor;
    else if (!strcmp(name, "Help"))
        slot = &helpNSCursor;
    else if (!strcmp(name, "ZoomIn"))
        slot = &zoomInNSCursor;
    else if (!strcmp(name, "ZoomOut"))
        slot = &zoomOutNSCursor;
    
    if (!slot)
        return nil;
    
    if (!*slot)
        *slot = [[coreCursorClass() alloc] init];
    return *slot;
WTF_ALLOW_UNSAFE_BUFFER_USAGE_END
}

#else

static NSCursor *cursor(const char *)
{
    return [NSCursor arrowCursor];
}

#endif // HAVE(HISERVICES)

// Simple NSCursor calls shouldn't need protection,
// but creating a cursor with a bad image might throw.

#if ENABLE(CUSTOM_CURSOR_SUPPORT)
#if ENABLE(MOUSE_CURSOR_SCALE)
static RetainPtr<NSCursor> createCustomCursor(Image* image, const IntPoint& hotSpot, float scale)
#else
static RetainPtr<NSCursor> createCustomCursor(Image* image, const IntPoint& hotSpot)
#endif
{
    // FIXME: The cursor won't animate.  Not sure if that's a big deal.
    auto nsImage = image->adapter().snapshotNSImage();
    if (!nsImage)
        return nullptr;
    BEGIN_BLOCK_OBJC_EXCEPTIONS

#if ENABLE(MOUSE_CURSOR_SCALE)
    NSSize size = NSMakeSize(image->width() / scale, image->height() / scale);
    NSSize expandedSize = NSMakeSize(ceil(size.width), ceil(size.height));

    // Pad the image with transparent pixels so it has an integer boundary.
    if (size.width != expandedSize.width || size.height != expandedSize.height) {
        RetainPtr<NSImage> expandedImage = adoptNS([[NSImage alloc] initWithSize:expandedSize]);
        NSRect toRect = NSMakeRect(0, expandedSize.height - size.height, size.width, size.height);
        NSRect fromRect = NSMakeRect(0, 0, image->width(), image->height());

        [expandedImage lockFocus];
        [nsImage drawInRect:toRect fromRect:fromRect operation:NSCompositingOperationSourceOver fraction:1];
        [expandedImage unlockFocus];

        return adoptNS([[NSCursor alloc] initWithImage:expandedImage.get() hotSpot:hotSpot]);
    }

    // Scale the image and its representation to match retina resolution.
    [nsImage setSize:expandedSize];
    [[[nsImage representations] objectAtIndex:0] setSize:expandedSize];
#endif

    return adoptNS([[NSCursor alloc] initWithImage:nsImage.get() hotSpot:hotSpot]);
    END_BLOCK_OBJC_EXCEPTIONS
    return nullptr;
}
#endif // ENABLE(CUSTOM_CURSOR_SUPPORT)

void Cursor::ensurePlatformCursor() const
{
    if (m_platformCursor)
        return;

    switch (m_type) {
    case Type::Pointer:
        m_platformCursor = [NSCursor arrowCursor];
        break;

    case Type::Cross:
        m_platformCursor = [NSCursor crosshairCursor];
        break;

    case Type::Hand:
        m_platformCursor = [NSCursor pointingHandCursor];
        break;

    case Type::IBeam:
        m_platformCursor = [NSCursor IBeamCursor];
        break;

    case Type::Wait:
        m_platformCursor = cursor("BusyButClickable");
        break;

    case Type::Help:
        m_platformCursor = cursor("Help");
        break;

    case Type::Move:
    case Type::MiddlePanning:
        m_platformCursor = cursor("Move");
        break;

    case Type::EastResize:
    case Type::EastPanning:
        m_platformCursor = cursor("ResizeEast");
        break;

    case Type::NorthResize:
    case Type::NorthPanning:
        m_platformCursor = cursor("ResizeNorth");
        break;

    case Type::NorthEastResize:
    case Type::NorthEastPanning:
        m_platformCursor = cursor("ResizeNortheast");
        break;

    case Type::NorthWestResize:
    case Type::NorthWestPanning:
        m_platformCursor = cursor("ResizeNorthwest");
        break;

    case Type::SouthResize:
    case Type::SouthPanning:
        m_platformCursor = cursor("ResizeSouth");
        break;

    case Type::SouthEastResize:
    case Type::SouthEastPanning:
        m_platformCursor = cursor("ResizeSoutheast");
        break;

    case Type::SouthWestResize:
    case Type::SouthWestPanning:
        m_platformCursor = cursor("ResizeSouthwest");
        break;

    case Type::WestResize:
    case Type::WestPanning:
        m_platformCursor = cursor("ResizeWest");
        break;

    case Type::NorthSouthResize:
        m_platformCursor = cursor("ResizeNorthSouth");
        break;

    case Type::EastWestResize:
        m_platformCursor = cursor("ResizeEastWest");
        break;

    case Type::NorthEastSouthWestResize:
        m_platformCursor = cursor("ResizeNortheastSouthwest");
        break;

    case Type::NorthWestSouthEastResize:
        m_platformCursor = cursor("ResizeNorthwestSoutheast");
        break;

    case Type::ColumnResize:
        m_platformCursor = [NSCursor resizeLeftRightCursor];
        break;

    case Type::RowResize:
        m_platformCursor = [NSCursor resizeUpDownCursor];
        break;

    case Type::VerticalText:
        m_platformCursor = [NSCursor IBeamCursorForVerticalLayout];
        break;

    case Type::Cell:
        m_platformCursor = cursor("Cell");
        break;

    case Type::ContextMenu:
        m_platformCursor = [NSCursor contextualMenuCursor];
        break;

    case Type::Alias:
        m_platformCursor = cursor("MakeAlias");
        break;

    case Type::Progress:
        m_platformCursor = cursor("BusyButClickable");
        break;

    case Type::NoDrop:
        m_platformCursor = [NSCursor operationNotAllowedCursor];
        break;

    case Type::Copy:
        m_platformCursor = [NSCursor dragCopyCursor];
        break;

    case Type::None:
#if ENABLE(CUSTOM_CURSOR_SUPPORT)
        m_platformCursor = adoptNS([[NSCursor alloc] initWithImage:adoptNS([[NSImage alloc] initWithSize:NSMakeSize(1, 1)]).get() hotSpot:NSZeroPoint]);
#else
        m_platformCursor = [NSCursor arrowCursor];
#endif
        break;

    case Type::NotAllowed:
        m_platformCursor = [NSCursor operationNotAllowedCursor];
        break;

    case Type::ZoomIn:
        m_platformCursor = cursor("ZoomIn");
        break;

    case Type::ZoomOut:
        m_platformCursor = cursor("ZoomOut");
        break;

    case Type::Grab:
        m_platformCursor = [NSCursor openHandCursor];
        break;

    case Type::Grabbing:
        m_platformCursor = [NSCursor closedHandCursor];
        break;

    case Type::Custom:
#if ENABLE(CUSTOM_CURSOR_SUPPORT)
#if ENABLE(MOUSE_CURSOR_SCALE)
        m_platformCursor = createCustomCursor(m_image.get(), m_hotSpot, m_imageScaleFactor);
#else
        m_platformCursor = createCustomCursor(m_image.get(), m_hotSpot);
#endif // ENABLE(MOUSE_CURSOR_SCALE)
#endif // ENABLE(CUSTOM_CURSOR_SUPPORT)
        break;

    case Type::Invalid:
    default:
        ASSERT_NOT_REACHED();
    }
}

NSCursor *Cursor::platformCursor() const
{
    ensurePlatformCursor();
    return m_platformCursor.get();
}

void Cursor::setAsPlatformCursor() const
{
    NSCursor *cursor = platformCursor();
    if ([NSCursor currentCursor] == cursor)
        return;
    [cursor set];
}

} // namespace WebCore

#endif // HAVE(NSCURSOR)
