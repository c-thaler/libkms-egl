/* Copyright (C) 2017 TES Electronic Solutions GmbH
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <linux/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <EGL/eglplatform.h>

#include "kms_egl.h"
#include "cdc_ioctl.h"

#include "kms_helper.h"


static void present(unsigned int _bufIdx, void *_gpuAddress, uint32_t _swapInterval);


static struct device *m_dev;
static int m_cdcFd;
static EGLNativeWindowTypeTES *m_window = NULL;


static EGLNativeWindowTypeTES *window_init(uint32_t width, uint32_t height, uint32_t format, uint32_t numBuffers)
{
   static EGLNativeWindowTypeTES win;

   win.width                  = width;
   win.height                 = height;
   win.format                 = format; /* 1=RGBA */
   win.num_buffers            = numBuffers;
   win.display_buffer_get_gpu = NULL;
   win.display_present        = &present;
   win.platform_window_handle = NULL;

   m_window = &win;

   return &win;
}


void *kms_egl_open(uint32_t _width, uint32_t _height, uint32_t _colorBits, uint32_t _numBuffers)
{
  EGLNativeWindowTypeTES *window;

  m_dev = drm_init();

  if (m_dev->fd == 0) {
    printf("Could not open KMS device!");
    return NULL;
  }

  if(!m_dev)
    printf("Failed to initialize KMS helper");

  drm_mode_setup(m_dev, 800, 600);

  window = window_init(_width, _height, 1 /*RGBA*/, _numBuffers);

  return window;
}


static void present(unsigned int _bufIdx, void *_gpuAddress, uint32_t _swapInterval)
{
static int i = 0;
  if (m_dev->fd == 0) {
    return;
  }

  struct hack_set_cb cb_arg;
  cb_arg.phy_addr = (void*) ((u_int32_t) _gpuAddress);
  cb_arg.width    = m_window->width;
  cb_arg.pitch    = m_window->width * 4;
  cb_arg.height   = m_window->height;

  ioctl(m_dev->fd, HACK_IOCTL_SET_CB, &cb_arg);

  if(0u != _swapInterval) {
    ioctl(m_dev->fd, HACK_IOCTL_WAIT_VSYNC, 0);
  }
}


void kms_egl_close(void* handle)
{
  if(handle != m_window)
    printf("CDC: Could not close handle: invalid handle\n");

  m_window = NULL;
}
