//
//  FuseThread.m
//  FuseZX
//
//  Created by Dieter Baron on 01.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

#import "Atari800Thread.h"

void atari800_main(int argc, char **argv);

@implementation Atari800Thread

- (void)main {
    const char *argv[256];
    
    int argc = (int)self.args.count;
    
    for (int i = 0; i < self.args.count; i++) {
        argv[i] = [self.args[i] cStringUsingEncoding:NSUTF8StringEncoding];
    }
    argv[argc] = NULL;
    
//    fuse_datadir = [[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"fuse"] cStringUsingEncoding:NSUTF8StringEncoding];
    
    joystickPort[0] = 0x1f;
    joystickPort[1] = 0x1f;

    display_init();
    atari800_main(argc, (char **)argv);
    display_fini();
}


@end

int joystickPort[2];

Atari800Thread *atari800Thread;


void atari800_main(int argc, char **argv) {
    /* initialise Atari800 core */
    int ret;
    if (!(ret=Atari800_Initialise(&argc, argv))) {
        printf("init failed: %d\n", ret);
        return;
    }

    printf("starting main loop\n");
    atari800Thread.running = YES;
    /* main loop */
    while (atari800Thread.running) {
        (void)[atari800Thread.delegate handleEvents];
        Atari800_Frame();
        if (Atari800_display_screen) {
            PLATFORM_DisplayScreen();
        }
    }
    
    Atari800_Exit(0);
}
