//
//  vice_c64.c
//  Vice-C64
//
//  Created by Dieter Baron on 17.06.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

#include <stdio.h>

// TODO: include correct file?
extern void c64model_set(int model);
extern void cartridge_trigger_freeze(void);

void model_set(int model) {
    c64model_set(model);
}

/* taken from https://www.pepto.de/projects/colorvic/ */
uint32_t palette[] = {
    0x000000FF,
    0xFFFFFFFF,
    0x813338FF,
    0x75CEC8FF,
    0x8E3C97FF,
    0x56AC4DFF,
    0x2E2C9BFF,
    0xEDF171FF,
    0x8E5029FF,
    0x553800FF,
    0xC46C71FF,
    0x4A4A4AFF,
    0x7B7B7BFF,
    0xA9FF9FFF,
    0x706DEBFF,
    0xB2B2B2FF
};
