/*
 Renderer.h -- Display Emulator Image, Optionally Hiding Border
 Copyright (C) 2019-2020 Dieter Baron
 
 This file is part of Ready, a home computer emulator for iPad.
 The authors can be contacted at <ready@tpau.group>.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 
 2. The names of the authors may not be used to endorse or promote
 products derived from this software without specific prior
 written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
 OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

@import UIKit;

typedef enum {
    BORDER_MODE_AUTO = 0,
    BORDER_MODE_SHOW = 1,
    BORDER_MODE_HIDE = 2,
} RendererBorderMode;

typedef struct {
    size_t x;
    size_t y;
} RendererPoint;

typedef struct {
    size_t width;
    size_t height;
} RendererSize;

typedef struct {
    RendererPoint origin;
    RendererSize size;
} RendererRect;

typedef struct {
    uint8_t * _Nonnull data;          // source image data, palette indices
    size_t rowSize;        // size of one row in data
    RendererSize size;     // size of image
} RendererImage;

void renderer_image_free(RendererImage * _Nullable image);
RendererImage * _Nullable renderer_image_new(size_t width, size_t height);

@protocol RendererDelegate <NSObject>

- (void)updateImage: (UIImage *_Nullable)image;

@end

@interface Renderer : NSObject {
    RendererRect _screenPosition;
}

/* MARK: - Public Methods */

- (void)resize: (const RendererSize)size;
- (void)close;

- (void)render: (const RendererImage *_Nonnull)image;
- (void)renderRGB: (const RendererImage *_Nonnull)image;

- (void)render:(const RendererImage * _Nonnull)image at: (RendererPoint)offset;
- (void)renderRGB:(const RendererImage * _Nonnull)image at: (RendererPoint)offset;

- (void)displayImage;

/* MARK: - Public Properties */

@property id _Nullable delegate;

@property RendererBorderMode borderMode; /* desired border mode*/
@property UInt32 * _Nullable palette; /* palette to use for rendering */

@property BOOL changed; /* bitmap changed since last displayImage */
@property RendererSize size; /* maximum size of image */
@property RendererRect screenPosition;   /* position of screen within full image */
@property RendererSize currentSize;    /* current size of rendered image */
@property RendererPoint currentOffset;  /* curront offset to source image du to border hiding */

/* MARK: - Private Methods */

- (RendererRect)clip:(const RendererImage * _Nonnull)image at:(RendererPoint)offset;
- (int)getBorderColor;
- (void)updateImage;

/* MARK: - Private Properties */

@property NSMutableData * _Nullable data; /* image data */
@property RendererBorderMode lastBorderMode; /* border mode of last render */

/* For border auto hiding */
@property int lastBorderColor;      /* color of the border in last frame*/
@property BOOL showBorder;   /* number of frames until transition */
@property int transitionCountdown;   /* number of frames until transition */
@property size_t borderWidth;        /* current widht of displayed border (used during transition animation) */

@end
