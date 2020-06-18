//
//  vice_c64.c
//  Vice-C64
//
//  Created by Dieter Baron on 17.06.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

#include <stdio.h>

// TODO: include correct file?
extern void vic20model_set(int model);

void model_set(int model) {
    vic20model_set(model);
}

void cartridge_trigger_freeze(void) {
}

uint32_t palette[] = {
    0x000000ff,
    0xffffffff,
    0x6d2327ff,
    0xa0fef8ff,
    0x8e3c97ff,
    0x7eda75ff,
    0x252390ff,
    0xffff86ff,
    0xa4643bff,
    0xffc8a1ff,
    0xf2a7abff,
    0xdbffffff,
    0xffb4ffff,
    0xd7ffceff,
    0x9d9affff,
    0xffffc9ff
};
