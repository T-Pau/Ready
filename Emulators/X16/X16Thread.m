//
//  FuseThread.m
//  FuseZX
//
//  Created by Dieter Baron on 01.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

#import "X16Thread.h"

@implementation X16Thread

- (void)main {
    const char *argv[256];
    
    int argc = (int)self.args.count;
    
    for (int i = 0; i < self.args.count; i++) {
        argv[i] = [self.args[i] cStringUsingEncoding:NSUTF8StringEncoding];
    }
    argv[argc] = NULL;
        
    x16_emulator_main(argc, (char **)argv);
}

@end

X16Thread *x16Thread;
