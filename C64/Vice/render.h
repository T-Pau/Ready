#ifndef HAD_RENDER_H
#define HAD_RENDER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    BORDER_MODE_AUTO = 0,
    BORDER_MODE_SHOW = 1,
    BORDER_MODE_HIDE = 2
} render_border_mode_t;

typedef struct {
    size_t x;
    size_t y;
} render_point_t;

typedef struct {
    size_t width;
    size_t height;
} render_size_t;

typedef struct {
    render_point_t origin;
    render_size_t size;
} render_rect_t;

typedef struct {
    uint8_t *data;          // source image data, palette indices
    size_t row_size;        // size of one row in data
    render_size_t size;     // size of image
    render_rect_t screen;   // rectangle of screen, excluding border
} render_image_t;

typedef struct render render_t;

const render_size_t *render(render_t *render, const render_image_t *image, render_border_mode_t border_mode);
size_t render_data_size(render_size_t size);
void render_free(render_t *render);
const render_size_t *render_get_bitmap_size(const render_t *render);
const render_point_t *render_get_current_offset(const render_t *render);
const render_rect_t *render_get_current_screen(const render_t *render);
const render_size_t *render_get_current_size(const render_t *render);
render_t *render_new(render_size_t size, void *data, const uint32_t *palette, render_border_mode_t border_mode);
void render_set_palette(render_t *render, const uint32_t *palette);

void render_image_free(render_image_t *image);
render_image_t *render_image_new(size_t width, size_t height);

#endif /* HAD_RENDER_H */
