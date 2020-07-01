//
//  FuseThread.m
//  FuseZX
//
//  Created by Dieter Baron on 01.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

#import "FuseThread.h"

#include "fuse.h"

@implementation FuseThread

- (id)init {
    if ((self = [super init]) == nil) {
        return nil;
    }
    self.qualityOfService = NSQualityOfServiceUserInteractive;
    return self;
}

- (void)main {
    const char *argv[256];
    argv[0] = "fuse";
    argv[1] = NULL;
    
    fuse_datadir = [[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"fuse"] cStringUsingEncoding:NSUTF8StringEncoding];

    fuse_main(1, (char **)argv);
}

@end
