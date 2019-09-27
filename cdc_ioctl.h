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

#ifndef CDC_IOCTL_H_
#define CDC_IOCTL_H_

struct hack_set_cb {
  void *phy_addr;
  int   width;
  int   pitch;
  int   height;
};

struct hack_set_winpos {
  int x;
  int y;
  int width;
  int height;
};

struct hack_set_alpha {
  int alpha;
};

#define HACK_IOCTL_BASE                  'h'
#define HACK_IO(nr)                      _IO(HACK_IOCTL_BASE,nr)
#define HACK_IOR(nr,type)                _IOR(HACK_IOCTL_BASE,nr,type)
#define HACK_IOW(nr,type)                _IOW(HACK_IOCTL_BASE,nr,type)
#define HACK_IOWR(nr,type)               _IOWR(HACK_IOCTL_BASE,nr,type)
#define HACK_IOCTL_NR(n)                 _IOC_NR(n)

#define HACK_IOCTL_VERSION               HACK_IO(0x00)

#define HACK_IOCTL_SET_CB                HACK_IOW(0xe0, struct hack_set_cb)
#define HACK_IOCTL_SET_WINPOS            HACK_IOW(0xe1, struct hack_set_winpos)
#define HACK_IOCTL_SET_ALPHA             HACK_IOW(0xe2, struct hack_set_alpha)
#define HACK_IOCTL_WAIT_VSYNC            HACK_IO( 0xe3)

#endif /* CDC_IOCTL_H_ */
