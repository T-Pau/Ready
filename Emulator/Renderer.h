//
//  Renderer.m
//  Emulator
//
//  Created by Dieter Baron on 15.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

@import UIKit;

typedef enum {
    BORDER_MODE_AUTO = 0,
    BORDER_MODE_SHOW = 1,
    BORDER_MODE_HIDE = 2
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
    uint8_t *data;          // source image data, palette indices
    size_t rowSize;        // size of one row in data
    RendererSize size;     // size of image
    RendererRect screen;   // rectangle of screen, excluding border
} RendererImage;

void renderer_image_free(RendererImage *image);
RendererImage *renderer_image_new(size_t width, size_t height);

@protocol RendererDelegate <NSObject>

- (void)updateImage: (UIImage *)image;

@end

@interface Renderer : NSObject

/* MARK: - Public Methods */

- (instancetype)initWithSize: (RendererSize)size borderMode: (RendererBorderMode)borderMode;

- (void)render: (const RendererImage *)image;
- (void)renderRGB: (const RendererImage *)image;

/* MARK: - Public Properties */

@property id delegate;

@property RendererBorderMode borderMode; /* desired border mode*/
@property UInt32 *palette; /* palette to use for rendering */

/* MARK: - Private Methods */

- (void)updateImage;

/* MARK: - Private Properties */

@property RendererSize size; /* maximum size of image */
@property NSMutableData *data; /* image data */
@property RendererBorderMode lastBorderMode; /* border mode of last render */
@property RendererSize currentSize;    /* current size of rendered image */
@property RendererPoint currentOffset;  /* curront offset to source image du to border hiding */
@property RendererRect currentScreen;   /* current position of screen with in image */

/* For border auto hiding */
@property int lastBorderColor;      /* color of the border in last frame*/
@property BOOL showBorder;   /* number of frames until transition */
@property int transitionCountdown;   /* number of frames until transition */
@property size_t borderWidth;        /* current widht of displayed border (used during transition animation) */

@end
