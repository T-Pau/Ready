//
//  Renderer.m
//  Emulator
//
//  Created by Dieter Baron on 15.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

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

- (instancetype)initWithSize:(RendererSize)size borderMode:(RendererBorderMode)borderMode {

    _size = size;
    _data = [[NSMutableData alloc] initWithLength:size.height * size.width * 4];
    _borderMode = borderMode;
    _lastBorderMode = borderMode;
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

    return self;
}

- (void)render:(const RendererImage *)image {
    if (image->size.width > _size.width || image->size.width > _size.width) {
        printf("image (%zu, %zu) bigger than bitmap (%zu, %zu)\n", image->size.width, image->size.height, _size.width, _size.height);
        return;
    }
    if (image->screen.origin.x + image->screen.size.width > image->size.width || image->screen.origin.y + image->screen.size.height > image->size.height) {
        printf("screen partly outside image\n");
        return;
    }
    
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
        int borderColor = get_border_color(image, _palette);
        
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
    
    size_t full_left_border = image->screen.origin.x;
    size_t full_right_border = image->size.width - (image->screen.origin.x + image->screen.size.width);
    size_t full_top_border = image->screen.origin.y;
    size_t full_bottom_border = image->size.height - (image->screen.origin.y + image->screen.size.height);
    
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
    
    _currentOffset.x = full_left_border - left_border;
    _currentOffset.y = full_top_border - top_border;
    _currentScreen.origin.x = left_border;
    _currentScreen.origin.y = top_border;
    _currentScreen.size = image->screen.size;
    _currentSize.width = left_border + image->screen.size.width + right_border;
    _currentSize.height = top_border + image->screen.size.height + bottom_border;
    
    size_t x0 = image->screen.origin.x - left_border;
    size_t y0 = image->screen.origin.y - top_border;
    
    const uint8_t *source = image->data + y0 * image->rowSize + x0;
    uint32_t *destination = [_data mutableBytes];
    
    for (size_t y = 0; y < _currentSize.height; y++) {
        for (size_t x = 0; x < _currentSize.width; x++) {
            destination[x] = _palette[source[x]];
        }
        
        source += image->rowSize;
        destination += _size.width;
    }
    
    [self updateImage];
}

- (void)renderRGB:(const RendererImage *)image {
    if (image->size.width > _size.width || image->size.width > _size.width) {
        printf("image (%zu, %zu) bigger than bitmap (%zu, %zu)\n", image->size.width, image->size.height, _size.width, _size.height);
        return;
    }

    const uint32_t *source = (const uint32_t *)image->data;
    uint32_t *destination = [_data mutableBytes];
    
    for (size_t y = 0; y < image->size.height; y++) {
        for (size_t x = 0; x < image->size.width; x++) {
            destination[x] = (source[x] << 8) | 0xff;
        }
        
        source += image->rowSize / sizeof(uint32_t);
        destination += _size.width;
    }
    
    _currentOffset.x = 0;
    _currentOffset.y = 0;
    _currentSize = image->size;
    
    [self updateImage];
}

- (void)updateImage {
    if (_delegate != nil) {
        @autoreleasepool {
            CIImage *ciImage = [CIImage imageWithBitmapData:_data bytesPerRow:_size.width * 4 size:CGSizeMake((CGFloat)_currentSize.width, (CGFloat)_currentSize.height) format:kCIFormatABGR8 colorSpace:CGColorSpaceCreateWithName(kCGColorSpaceSRGB)];
            
            UIImage *image = [UIImage imageWithCIImage:ciImage];
            dispatch_async(dispatch_get_main_queue(), ^{
                [self.delegate updateImage:image];
            });
        }
    }
}

@end


static int get_border_color(const RendererImage *image, const uint32_t *palette) {
    
    int border_color = BORDER_COLOR_UNKNOWN;
    
    for (size_t y = 0; y < image->size.height; y++) {
        for (size_t x = 0; x < image->size.width; x++) {
            if (x == image->screen.origin.x && y >= image->screen.origin.y && y < image->screen.origin.y + image->screen.size.height) {
                x = image->screen.origin.x + image->screen.size.width;
            }
            int color = image->data[x + y * image->rowSize];
            if (border_color == BORDER_COLOR_UNKNOWN) {
                border_color = color;
            }
            else if (border_color != color) {
                return BORDER_COLOR_MULTIPLE;
            }
        }
    }
    
    if (border_color == BORDER_COLOR_UNKNOWN) {
        return border_color;
    }
    return palette[border_color];
}


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
    image->screen.origin.x = 0;
    image->screen.origin.y = 0;
    image->screen.size.width = width;
    image->screen.size.height = height;
    
    return image;
}
