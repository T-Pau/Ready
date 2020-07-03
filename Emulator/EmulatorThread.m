//
//  EmulatorThread.m
//  Emulator
//
//  Created by Dieter Baron on 03.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

@import UIKit;

#include "EmulatorThread.h"

@implementation EmulatorThread

- (id)init {
    if ((self = [super init]) == nil) {
        return nil;
    }
    self.qualityOfService = NSQualityOfServiceUserInteractive;
    return self;
}


- (void)initBitmapWidth:(size_t)width height:(size_t)height {
    self.bytesPerRow = width * 4;
    self.imageData = [NSMutableData dataWithLength:self.bytesPerRow * height];
}


- (void)updateBitmapWidth: (size_t)width height: (size_t)height {
    @autoreleasepool {
        CIImage *ciImage = [CIImage imageWithBitmapData:_imageData bytesPerRow:_bytesPerRow size:CGSizeMake((CGFloat)width, (CGFloat)height) format:kCIFormatABGR8 colorSpace:CGColorSpaceCreateWithName(kCGColorSpaceSRGB)];
        
        [self.delegate updateImage: [UIImage imageWithCIImage:ciImage]];
    }
}

@end
