#ifndef VICE_VIDEOARCH_H
#define VICE_VIDEOARCH_H

/*
 videoarch.h -- iOS Video Definition
 Copyright (C) 2019 Dieter Baron
 
 This file is part of C64, a Commodore 64 emulator for iOS, based on VICE.
 The authors can be contacted at <c64@spiderlab.at>
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307  USA.
*/

#include <stdint.h>
#include "viewport.h"

typedef struct video_canvas_s {
    /** \brief Nonzero if it is safe to access other members of the
     *         structure. */
    unsigned int initialized;
    /** \brief Nonzero if the structure has been fully realized. */
    unsigned int created;
    
    // TODO: [ios] fill in video_canvas_t
    
    /** \brief Rendering configuration as seen by the emulator
     *         core. */
    struct video_render_config_s *videoconfig;

    /** \brief Drawing buffer as seen by the emulator core. */
    struct draw_buffer_s *draw_buffer;

    /** \brief Display window as seen by the emulator core. */
    struct viewport_s *viewport;

    /** \brief Methods for managing the draw buffer when the core
     *         rasterizer handles it. */
    struct video_draw_buffer_callback_s *video_draw_buffer_callback;
    
    /** \brief Machine screen geometry as seen by the emulator
     *         core. */
    struct geometry_s *geometry;

    /** \brief Color palette for translating display results into
     *         window colors. */
    struct palette_s *palette;

    size_t index;
} video_canvas_t;

extern int canvas_has_partial_updates;

#endif /* VICE_VIDEOARCH_H */
