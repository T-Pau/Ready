/*
 * ringbuffer.c - C ring buffer (FIFO) implementation.
 *
 * Written in 2011 by Drew Hess <dhess-src@bothan.net>.
 *
 * To the extent possible under law, the author(s) have dedicated all
 * copyright and related and neighboring rights to this software to
 * the public domain worldwide. This software is distributed without
 * any warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication
 * along with this software. If not, see
 * <http://creativecommons.org/publicdomain/zero/1.0/>.
 */

#include "ringbuffer.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/param.h>
#include <assert.h>

/*
 * The code is written for clarity, not cleverness or performance, and
 * contains many assert()s to enforce invariant assumptions and catch
 * bugs. Feel free to optimize the code and to remove asserts for use
 * in your own projects, once you're comfortable that it functions as
 * intended.
 */

struct ringbuffer
{
    uint8_t *buf;
    uint8_t *head, *tail;
    size_t size;
};

ringbuffer_t *
ringbuffer_new(size_t capacity)
{
    ringbuffer_t *rb = malloc(sizeof(struct ringbuffer));
    if (rb) {

        /* One byte is used for detecting the full condition. */
        rb->size = capacity + 1;
        rb->buf = malloc(rb->size);
        if (rb->buf)
            ringbuffer_reset(rb);
        else {
            free(rb);
            return 0;
        }
    }
    return rb;
}

size_t
ringbuffer_buffer_size(const ringbuffer_t *rb)
{
    return rb->size;
}

void
ringbuffer_reset(ringbuffer_t *rb)
{
    rb->head = rb->tail = rb->buf;
}

void
ringbuffer_free(ringbuffer_t *rb)
{
    if (rb == NULL) {
        return;
    }
    free(rb->buf);
    free(rb);
}

size_t
ringbuffer_capacity(const ringbuffer_t *rb)
{
    return ringbuffer_buffer_size(rb) - 1;
}

/*
 * Return a pointer to one-past-the-end of the ring buffer's
 * contiguous buffer. You shouldn't normally need to use this function
 * unless you're writing a new ringbuffer_* function.
 */
static const uint8_t *
ringbuffer_end(const ringbuffer_t *rb)
{
    return rb->buf + ringbuffer_buffer_size(rb);
}

size_t
ringbuffer_bytes_free(const ringbuffer_t *rb)
{
    if (rb->head >= rb->tail)
        return ringbuffer_capacity(rb) - (rb->head - rb->tail);
    else
        return rb->tail - rb->head - 1;
}

size_t
ringbuffer_bytes_used(const ringbuffer_t *rb)
{
    return ringbuffer_capacity(rb) - ringbuffer_bytes_free(rb);
}

int
ringbuffer_is_full(const ringbuffer_t *rb)
{
    return ringbuffer_bytes_free(rb) == 0;
}

int
ringbuffer_is_empty(const ringbuffer_t *rb)
{
    return ringbuffer_bytes_free(rb) == ringbuffer_capacity(rb);
}

const void *
ringbuffer_tail(const ringbuffer_t *rb)
{
    return rb->tail;
}

const void *
ringbuffer_head(const ringbuffer_t *rb)
{
    return rb->head;
}

/*
 * Given a ring buffer rb and a pointer to a location within its
 * contiguous buffer, return the a pointer to the next logical
 * location in the ring buffer.
 */
static uint8_t *
ringbuffer_nextp(ringbuffer_t *rb, const uint8_t *p)
{
    /*
     * The assert guarantees the expression (++p - rb->buf) is
     * non-negative; therefore, the modulus operation is safe and
     * portable.
     */
    //assert((p >= rb->buf) && (p < ringbuffer_end(rb)));
    return rb->buf + ((++p - rb->buf) % ringbuffer_buffer_size(rb));
}

size_t
ringbuffer_findchr(const ringbuffer_t *rb, int c, size_t offset)
{
    const uint8_t *bufend = ringbuffer_end(rb);
    size_t bytes_used = ringbuffer_bytes_used(rb);
    if (offset >= bytes_used)
        return bytes_used;

    const uint8_t *start = rb->buf +
        (((rb->tail - rb->buf) + offset) % ringbuffer_buffer_size(rb));
    //assert(bufend > start);
    size_t n = MIN(bufend - start, bytes_used - offset);
    const uint8_t *found = memchr(start, c, n);
    if (found)
        return offset + (found - start);
    else
        return ringbuffer_findchr(rb, c, offset + n);
}

size_t
ringbuffer_memset(ringbuffer_t *dst, int c, size_t len)
{
    const uint8_t *bufend = ringbuffer_end(dst);
    size_t nwritten = 0;
    size_t count = MIN(len, ringbuffer_buffer_size(dst));
    int overflow = count > ringbuffer_bytes_free(dst);

    while (nwritten != count) {

        /* don't copy beyond the end of the buffer */
        //assert(bufend > dst->head);
        size_t n = MIN(bufend - dst->head, count - nwritten);
        memset(dst->head, c, n);
        dst->head += n;
        nwritten += n;

        /* wrap? */
        if (dst->head == bufend)
            dst->head = dst->buf;
    }

    if (overflow) {
        dst->tail = ringbuffer_nextp(dst, dst->head);
        //assert(ringbuffer_is_full(dst));
    }

    return nwritten;
}

void *
ringbuffer_memcpy_into(ringbuffer_t *dst, const void *src, size_t count)
{
    const uint8_t *u8src = src;
    const uint8_t *bufend = ringbuffer_end(dst);
    int overflow = count > ringbuffer_bytes_free(dst);
    size_t nread = 0;

    while (nread != count) {
        /* don't copy beyond the end of the buffer */
        //assert(bufend > dst->head);
        size_t n = MIN(bufend - dst->head, count - nread);
        memcpy(dst->head, u8src + nread, n);
        dst->head += n;
        nread += n;

        /* wrap? */
        if (dst->head == bufend)
            dst->head = dst->buf;
    }

    if (overflow) {
        dst->tail = ringbuffer_nextp(dst, dst->head);
        //assert(ringbuffer_is_full(dst));
    }

    return dst->head;
}

ssize_t
ringbuffer_read(int fd, ringbuffer_t *rb, size_t count)
{
    const uint8_t *bufend = ringbuffer_end(rb);
    size_t nfree = ringbuffer_bytes_free(rb);

    /* don't write beyond the end of the buffer */
    //assert(bufend > rb->head);
    count = MIN(bufend - rb->head, count);
    ssize_t n = read(fd, rb->head, count);
    if (n > 0) {
        //assert(rb->head + n <= bufend);
        rb->head += n;

        /* wrap? */
        if (rb->head == bufend)
            rb->head = rb->buf;

        /* fix up the tail pointer if an overflow occurred */
        if (n > nfree) {
            rb->tail = ringbuffer_nextp(rb, rb->head);
            //assert(ringbuffer_is_full(rb));
        }
    }

    return n;
}

void *
ringbuffer_memcpy_from(void *dst, ringbuffer_t *src, size_t count)
{
    size_t bytes_used = ringbuffer_bytes_used(src);
    if (count > bytes_used)
        return 0;

    uint8_t *u8dst = dst;
    const uint8_t *bufend = ringbuffer_end(src);
    size_t nwritten = 0;
    while (nwritten != count) {
        //assert(bufend > src->tail);
        size_t n = MIN(bufend - src->tail, count - nwritten);
        memcpy(u8dst + nwritten, src->tail, n);
        src->tail += n;
        nwritten += n;

        /* wrap ? */
        if (src->tail == bufend)
            src->tail = src->buf;
    }

    //assert(count + ringbuffer_bytes_used(src) == bytes_used);
    return src->tail;
}

ssize_t
ringbuffer_write(int fd, ringbuffer_t *rb, size_t count)
{
    size_t bytes_used = ringbuffer_bytes_used(rb);
    if (count > bytes_used)
        return 0;

    const uint8_t *bufend = ringbuffer_end(rb);
    //assert(bufend > rb->head);
    count = MIN(bufend - rb->tail, count);
    ssize_t n = write(fd, rb->tail, count);
    if (n > 0) {
        //assert(rb->tail + n <= bufend);
        rb->tail += n;

        /* wrap? */
        if (rb->tail == bufend)
            rb->tail = rb->buf;

        //assert(n + ringbuffer_bytes_used(rb) == bytes_used);
    }

    return n;
}

void *
ringbuffer_copy(ringbuffer_t *dst, ringbuffer_t *src, size_t count)
{
    size_t src_bytes_used = ringbuffer_bytes_used(src);
    if (count > src_bytes_used)
        return 0;
    int overflow = count > ringbuffer_bytes_free(dst);

    const uint8_t *src_bufend = ringbuffer_end(src);
    const uint8_t *dst_bufend = ringbuffer_end(dst);
    size_t ncopied = 0;
    while (ncopied != count) {
        //assert(src_bufend > src->tail);
        size_t nsrc = MIN(src_bufend - src->tail, count - ncopied);
        //assert(dst_bufend > dst->head);
        size_t n = MIN(dst_bufend - dst->head, nsrc);
        memcpy(dst->head, src->tail, n);
        src->tail += n;
        dst->head += n;
        ncopied += n;

        /* wrap ? */
        if (src->tail == src_bufend)
            src->tail = src->buf;
        if (dst->head == dst_bufend)
            dst->head = dst->buf;
    }

    //assert(count + ringbuffer_bytes_used(src) == src_bytes_used);
    
    if (overflow) {
        dst->tail = ringbuffer_nextp(dst, dst->head);
        //assert(ringbuffer_is_full(dst));
    }

    return dst->head;
}
