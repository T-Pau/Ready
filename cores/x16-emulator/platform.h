//
//  platform.h
//  Ready
//
//  Created by Dieter Baron on 15.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

#ifndef HAD_PLATFORM_H
#define HAD_PLATFORM_H

size_t platform_audio_available(void);
void platform_audio_close(void);
int platform_audio_init(int samplerate, int samples_per_buffer, int channels);
void platform_audio_write(void *buffer, size_t length);

bool platform_video_update(const uint32_t *framebuffer);

#endif /* HAD_PLATFORM_H */
