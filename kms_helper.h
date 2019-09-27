/*
 *Copyright (C) 2016 TES Electronic Solutions GmbH
 *
 *Permission is hereby granted, free of charge, to any person obtaining a copy
 *of this software and associated documentation files (the "Software"), to deal
 *in the Software without restriction, including without limitation the rights
 *to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *copies of the Software, and to permit persons to whom the Software is
 *furnished to do so, subject to the following conditions:
 *
 *The above copyright notice and this permission notice shall be included in
 *all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *THE SOFTWARE.
 */

#ifndef KMS_HELPER_H_
#define KMS_HELPER_H_

#include <stdint.h>
#include <libkms/libkms.h>

struct resources;

struct device {
    int fd;
    struct kms_driver *kms_driver;

    struct resources *resources;

    struct {
        unsigned int width;
        unsigned int height;

        unsigned int off_x;
        unsigned int off_y;

        unsigned int fb_id;
    } mode;

    struct {
      unsigned int width;
      unsigned int height;

      uint32_t crtc_id;
      int bo_handle;
      char *data;
    } cursor;
};


struct device *drm_init(void);
int drm_mode_setup(struct device *dev, uint16_t hdisplay, uint16_t vdisplay);
unsigned int drm_get_physical_width_mm(struct device *dev);
unsigned int drm_get_physical_height_mm(struct device *dev);
int drm_cursor_create(struct device *dev, uint16_t width, uint16_t height);
void drm_cursor_move(struct device *dev, uint16_t x, uint16_t y);
void drm_cursor_image(struct device *dev, const unsigned char* data, unsigned int size);

#endif /* KMS_HELPER_H_ */
