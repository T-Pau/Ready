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

- (void)main {
    const char *argv[256];

    int argc = (int)self.args.count;
    
    for (int i = 0; i < self.args.count; i++) {
        argv[i] = [self.args[i] cStringUsingEncoding:NSUTF8StringEncoding];
    }
    argv[argc] = NULL;
    
    fuse_datadir = [[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"fuse"] cStringUsingEncoding:NSUTF8StringEncoding];

    fuse_main(argc, (char **)argv);
}


@end

FuseThread *fuseThread;
