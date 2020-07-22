/*
 Renderer.m -- Display Emulator Image, Optionally Hiding Border
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

#import "Renderer.h"

#define MY_MIN(a, b) ((a) < (b) ? (a) : (b))

#define BORDER_COLOR_UNKNOWN (-1)
#define BORDER_COLOR_MULTIPLE (-2)

#define BORDER_WIDTH_MAX 0xffff


/* For border auto hiding */
/* Number of consecutive frames border must not change until it's hidden */
#define BORDER_HIDE_FRAMES 25
/* Number of consecutive frames border must change until it's shown */
#define BORDER_SHOW_FRAMES 5


static size_t border_animation[] = {
    1, 1,
    2, 2,  2, 2,
    3, 3, 3,  3, 3, 3,  3, 3, 3,
    4
};
static size_t num_animation = sizeof(border_animation) / sizeof(border_animation[0]);

static int get_border_color(const RendererImage *image, const uint32_t *palette);


@implementation Renderer

- (instancetype)init {
    _size.width = 0;
    _size.height = 0;
    _changed = NO;
    
    return self;
}

- (void)resize:(const RendererSize)size {
    [self resize:size doubleLines:NO];
}
- (void)resize:(const RendererSize)size doubleLines:(BOOL)doubleLines {
    _size = size;
    _doubleLines = doubleLines;
    size_t dataLength = (size.height + 1) * size.width * sizeof(uint32_t) * (doubleLines ? 2 : 1);
    _data = [[NSMutableData alloc] initWithLength:dataLength];
    _lastBorderMode = _borderMode;
    _lastBorderColor = BORDER_COLOR_UNKNOWN;
    if (_borderMode == BORDER_MODE_SHOW) {
        _showBorder = true;
        _transitionCountdown = BORDER_HIDE_FRAMES;
    }
    else {
        _showBorder = false;
        _transitionCountdown = BORDER_SHOW_FRAMES;
    }
    _borderWidth = BORDER_WIDTH_MAX;
}

- (void)close {
    _size.width = 0;
    _size.height = 0;
    _data = nil;
}

- (RendererRect)screenPosition {
    return _screenPosition;
}

- (void)setScreenPosition:(RendererRect)screenPosition {
    if (screenPosition.origin.x + screenPosition.size.width > _size.width || screenPosition.origin.y + screenPosition.size.height > _size.height) {
        printf("screen partly outside image\n");
        return;
    }
    _screenPosition = screenPosition;
}

- (RendererRect)clip:(const RendererImage *)image at:(RendererPoint)offset {
    RendererRect rect;

    rect.origin.x = MY_MIN(offset.x, _size.width);
    rect.origin.y = MY_MIN(offset.y, _size.height);
    rect.size.width = MY_MIN(image->size.width, _size.width - rect.origin.x);
    rect.size.height = MY_MIN(image->size.height, _size.height - rect.origin.y);

    return rect;
}

- (void)render:(const RendererImage *)image {
    RendererPoint offset = {0, 0};
    [self render:image at:offset];
}

- (void)render:(const RendererImage *)image at:(RendererPoint)offset {
    RendererRect rect = [self clip:image at:offset];

    const uint8_t *source = image->data;
    uint32_t *destination = [self dataAt:rect.origin];
    
    for (size_t y = 0; y < rect.size.height; y++) {
        BOOL lineChanged = NO;
        for (size_t x = 0; x < rect.size.width; x++) {
            uint32_t value = _palette[source[x]];
            if (destination[x] != value) {
                lineChanged = YES;
                destination[x] = value;
            }
        }
        
        if (_doubleLines) {
            if (lineChanged) {
                memcpy(destination + _size.width, destination, rect.size.width * sizeof(destination[0]));
            }
            destination += _size.width;
        }
        
        _changed |= lineChanged;
        source += image->rowSize;
        destination += _size.width;
    }
}

- (void)renderRGB:(const RendererImage *)image {
    RendererPoint offset = {0, 0};
    [self renderRGB:image at:offset];
}


- (void)renderRGB:(const RendererImage *)image at:(RendererPoint)offset {
    RendererRect rect = [self clip:image at:offset];

    const uint32_t *source = (const uint32_t *)image->data;
    uint32_t *destination = [self dataAt:rect.origin];

    for (size_t y = 0; y < rect.size.height; y++) {
        BOOL lineChanged = NO;
        for (size_t x = 0; x < rect.size.width; x++) {
            uint32_t value = (source[x] << 8) | 0xff;
            if (destination[x] != value) {
                lineChanged = YES;
                destination[x] = value;
            }
        }

        if (_doubleLines && lineChanged) {
            memcpy(destination + _size.width, destination, rect.size.width * sizeof(destination[0]));
            destination += _size.width;
        }

        _changed |= lineChanged;
        source += image->rowSize / sizeof(uint32_t);
        destination += _size.width;
    }
}


- (void)displayImage {
    if (_lastBorderMode != _borderMode) {
        switch (_borderMode) {
        case BORDER_MODE_AUTO:
            if (_showBorder) {
                if (_lastBorderColor >= 0) {
                    _showBorder = false;
                    _transitionCountdown = BORDER_SHOW_FRAMES;
                }
                else {
                    _transitionCountdown = BORDER_HIDE_FRAMES;
                }
            }
            else {
                if (_lastBorderColor >= 0) {
                    _transitionCountdown = BORDER_SHOW_FRAMES;
                }
                else {
                    _showBorder = true;
                    _transitionCountdown = BORDER_HIDE_FRAMES;
                }
            }
            break;
            
        case BORDER_MODE_HIDE:
            _showBorder = false;
            break;
            
        case BORDER_MODE_SHOW:
            _showBorder = true;
            break;
        }
    }
    
    _lastBorderMode = _borderMode;
    
    if (_borderMode == BORDER_MODE_AUTO) {
        int borderColor = _changed ? [self getBorderColor] : _lastBorderColor;
        
        bool borderChanged = (borderColor == BORDER_COLOR_MULTIPLE || borderColor != _lastBorderColor);
        _lastBorderColor = borderColor;
        
        if (_showBorder == borderChanged) {
            /* reset transition countdown */
            _transitionCountdown = borderChanged ? BORDER_MODE_HIDE : BORDER_SHOW_FRAMES;
        }
        else {
            _transitionCountdown -= 1;
            if (_transitionCountdown == 0) {
                _showBorder = !_showBorder;
                _transitionCountdown = _showBorder ? BORDER_HIDE_FRAMES : BORDER_SHOW_FRAMES;
            }
        }
    }
    
    size_t full_left_border = _screenPosition.origin.x;
    size_t full_right_border = _size.width - (_screenPosition.origin.x + _screenPosition.size.width);
    size_t full_top_border = _screenPosition.origin.y;
    size_t full_bottom_border = _size.height - (_screenPosition.origin.y + _screenPosition.size.height);
    
    size_t border_max = full_left_border;
    if (full_right_border > border_max) {
        border_max = full_right_border;
    }
    if (full_top_border > border_max) {
        border_max = full_top_border;
    }
    if (full_bottom_border > border_max) {
        border_max = full_bottom_border;
    }
    
    if (_borderWidth == BORDER_WIDTH_MAX) {
        if (_showBorder) {
            _borderWidth = border_max;
        }
        else {
            _borderWidth = 0;
        }
    }
    
    size_t diff = MY_MIN(_borderWidth, border_max - _borderWidth);
    size_t step = border_animation[MY_MIN(diff, num_animation - 1)];
    
    if (_showBorder) {
        if (_borderWidth < border_max) {
            _borderWidth += step;
        }
    }
    else {
        if (_borderWidth > 0) {
            _borderWidth -= step;
        }
    }
    
    size_t left_border = MY_MIN(full_left_border, _borderWidth);
    size_t right_border = MY_MIN(full_right_border, _borderWidth);
    size_t top_border = MY_MIN(full_top_border, _borderWidth);
    size_t bottom_border = MY_MIN(full_bottom_border, _borderWidth);
    
    RendererRect rect;
    rect.origin.x = full_left_border - left_border;
    rect.origin.y = full_top_border - top_border;
    rect.size.width = left_border + _screenPosition.size.width + right_border;
    rect.size.height = top_border + _screenPosition.size.height + bottom_border;
    
    BOOL needsUpdate = _changed || rect.origin.x != _currentOffset.x || rect.origin.y != _currentOffset.y || rect.size.width != _currentSize.width || rect.size.height != _currentSize.height;

    _changed = NO;
    _currentOffset = rect.origin;
    _currentSize = rect.size;

    if (needsUpdate) {
        [self updateImage];
    }
}


- (void)updateImage {
    if (_delegate != nil) {
        @autoreleasepool {
            NSUInteger start = (uint8_t *)[self dataAt:_currentOffset] - (uint8_t *)[_data mutableBytes];
            NSRange range = NSMakeRange(start, [_data length] - start);
            CIImage *ciImage = [CIImage imageWithBitmapData:[_data subdataWithRange:range] bytesPerRow:_size.width * 4 size:CGSizeMake((CGFloat)_currentSize.width, (CGFloat)_currentSize.height * (_doubleLines ? 2 : 1)) format:kCIFormatABGR8 colorSpace:CGColorSpaceCreateWithName(kCGColorSpaceSRGB)];
            
            UIImage *image = [UIImage imageWithCIImage:ciImage];
            dispatch_async(dispatch_get_main_queue(), ^{
                [self.delegate renderer:self updateImage:image];
            });
        }
    }
}

- (uint32_t *)dataAt:(RendererPoint)offset {
    return (uint32_t *)[_data mutableBytes] + offset.y * _size.width * (_doubleLines ? 2 : 1) + offset.x;
}
- (int)getBorderColor {
    int border_color = BORDER_COLOR_UNKNOWN;
    
    const uint32_t *data = [_data bytes];
    
    for (size_t y = 0; y < _size.height; y++) {
        for (size_t x = 0; x < _size.width; x++) {
            if (x == _screenPosition.origin.x && y >= _screenPosition.origin.y && y < _screenPosition.origin.y + _screenPosition.size.height) {
                x = _screenPosition.origin.x + _screenPosition.size.width;
            }
            int color = data[x + y * _size.width];
            if (border_color == BORDER_COLOR_UNKNOWN) {
                border_color = color;
            }
            else if (border_color != color) {
                return BORDER_COLOR_MULTIPLE;
            }
        }
    }
    
    return border_color;
}

@end


void renderer_image_free(RendererImage *image) {
    if (image) {
        free(image->data);
        free(image);
    }
}

RendererImage *renderer_image_new(size_t width, size_t height) {
    RendererImage *image;
    
    if ((image = malloc(sizeof(*image))) == NULL) {
        return NULL;
    }
    if ((image->data = calloc(width * height, 1)) == NULL) {
        free(image);
        return NULL;
    }
    
    image->rowSize = width;
    image->size.width = width;
    image->size.height = height;
    
    return image;
}
