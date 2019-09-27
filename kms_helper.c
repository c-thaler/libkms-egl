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

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <libkms/libkms.h>

#include "kms_helper.h"


struct crtc {
    drmModeCrtc *crtc;
    drmModeObjectProperties *props;
    drmModePropertyRes **props_info;
    drmModeModeInfo *mode;
};


struct encoder {
    drmModeEncoder *encoder;
};


struct connector {
    drmModeConnector *connector;
    drmModeObjectProperties *props;
    drmModePropertyRes **props_info;
};


struct fb {
    drmModeFB *fb;
};


struct plane {
    drmModePlane *plane;
    drmModeObjectProperties *props;
    drmModePropertyRes **props_info;
};


struct resources {
    drmModeRes *res;
    drmModePlaneRes *plane_res;

    struct crtc crtc;
    struct encoder encoder;
    struct connector connector;
    struct fb fb;
    struct plane plane;
};


void create_bo(struct kms_driver *kms_driver,
    int w, int h, int *out_pitch, struct kms_bo **out_kms_bo,
    int *out_handle)
{
    void *map_buf;
    struct kms_bo *bo;
    int pitch, handle;
    unsigned bo_attribs[] = {
        KMS_WIDTH,   w,
        KMS_HEIGHT,  h,
        KMS_BO_TYPE, KMS_BO_TYPE_SCANOUT_X8R8G8B8,
        KMS_TERMINATE_PROP_LIST
    };
    int ret;

    /* ceate kms buffer object, opaque struct identied by struct kms_bo pointer */
    ret = kms_bo_create(kms_driver, bo_attribs, &bo);
    if(ret){
        fprintf(stderr, "kms_bo_create failed: %s\n", strerror(errno));
        goto exit;
    }

    /* get the "pitch" or "stride" of the bo */
    ret = kms_bo_get_prop(bo, KMS_PITCH, &pitch);
    if(ret){
        fprintf(stderr, "kms_bo_get_prop KMS_PITCH failed: %s\n", strerror(errno));
        goto free_bo;
    }

    /* get the handle of the bo */
    ret = kms_bo_get_prop(bo, KMS_HANDLE, &handle);
    if(ret){
        fprintf(stderr, "kms_bo_get_prop KMS_HANDL failed: %s\n", strerror(errno));
        goto free_bo;
    }

    /* map the bo to user space buffer */
    ret = kms_bo_map(bo, &map_buf);
    if(ret){
        fprintf(stderr, "kms_bo_map failed: %s\n", strerror(errno));
        goto free_bo;
    }

    kms_bo_unmap(bo);

    ret = 0;
    *out_kms_bo = bo;
    *out_pitch = pitch;
    *out_handle = handle;
    goto exit;

free_bo:
    kms_bo_destroy(&bo);

exit:
    return;

}


drmModeConnector* find_connector(int fd, drmModeRes *res)
{
  int i;
  drmModeConnector *connector;

  for(i=0; i < res->count_connectors; ++i)
  {
    connector = drmModeGetConnector(fd, res->connectors[i]);
    if(connector != NULL){
      fprintf(stderr, "connector %d found\n", connector->connector_id);
      if(connector->connection == DRM_MODE_CONNECTED && connector->count_modes > 0)
        break;
      drmModeFreeConnector(connector);
    }
    else
      fprintf(stderr, "get a null connector pointer\n");
  }

  if(i == res->count_connectors)
  {
    fprintf(stderr, "No active connector found.\n");
    return NULL;
  }

  return connector;
}


drmModeEncoder* find_encoder_for_connector(int fd, drmModeRes *res, drmModeConnector *connector)
{
  drmModeEncoder *encoder;
  int i;

  for(i=0; i < res->count_encoders; ++i)
  {
    encoder = drmModeGetEncoder(fd, res->encoders[i]);
    if(encoder != NULL){
      fprintf(stderr, "encoder %d found\n", encoder->encoder_id);
      if(encoder->encoder_id == connector->encoder_id);
        break;
      drmModeFreeEncoder(encoder);
    } else
      fprintf(stderr, "get a null encoder pointer\n");
  }

  if(i == res->count_encoders){
    fprintf(stderr, "No matching encoder with connector, shouldn't happen\n");
    return NULL;
  }

  return encoder;
}


drmModeCrtcPtr mode_set(struct device *dev, drmModeConnector *connector, drmModeEncoder *encoder, int fb_id, drmModeModeInfo *mode, int pplane_id, uint16_t plane_width, uint16_t plane_height)
{
  drmModeCrtcPtr crtc;
  int ret;

  crtc = drmModeGetCrtc(dev->fd, encoder->crtc_id);
  if (crtc == NULL)
    return NULL;

  dev->mode.off_x = (mode->hdisplay - plane_width) / 2;
  dev->mode.off_y = (mode->vdisplay - plane_height) / 2;

  drmModeSetPlane(dev->fd, pplane_id, encoder->crtc_id,
		  fb_id, 0,
		  dev->mode.off_x, dev->mode.off_y,
		  plane_width, plane_height,
		  0,0,
		  plane_width, plane_height);

  ret = drmModeSetCrtc(
              dev->fd, encoder->crtc_id, fb_id,
              0, 0,   /* x, y */
              &connector->connector_id,
              1,      /* element count of the connectors array above*/
              mode);
  if(ret){
    fprintf(stderr, "drmModeSetCrtc failed: %s\n", strerror(errno));
    return NULL;
  }

  return crtc;
}


struct device * drm_init(void)
{
  struct device *dev;
  struct kms_bo *kms_bo, *second_kms_bo;
  drmModeModeInfo mode;
  char *module = "tes-cdc";
  int ret = 0;
  uint64_t cap = 0;
  int i;

  dev = malloc(sizeof *dev);
  memset(dev, 0, sizeof *dev);
  dev->resources = malloc(sizeof *dev->resources);

  dev->fd = drmOpen(module, NULL);
  if (dev->fd < 0)
  {
    printf("failed to open device '%s' (err=%d).\n", module, dev->fd);
    return NULL;
  }
  else
  {
    printf("successfully opened device '%s'.\n", module);
  }

  drmSetClientCap(dev->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);

  dev->resources->res = drmModeGetResources(dev->fd);
  dev->resources->plane_res = drmModeGetPlaneResources(dev->fd);
  dev->resources->connector.connector = find_connector(dev->fd, dev->resources->res);
  dev->resources->encoder.encoder = find_encoder_for_connector(dev->fd, dev->resources->res, dev->resources->connector.connector);

  ret = drmGetCap(dev->fd, DRM_CAP_DUMB_BUFFER, &cap);
  if (ret || cap == 0)
  {
    fprintf(stderr, "no dumb buffer support (err=%d)\n", ret);
    return NULL;
  }

  /* init kms bo stuff */
  ret = kms_create(dev->fd, &dev->kms_driver);
  if(ret)
  {
    fprintf(stderr, "kms_create failed: %s\n", strerror(errno));
    return NULL;
  }

  fprintf(stderr, "Found %d planes:\n", dev->resources->plane_res->count_planes);
  for(i=0; i < dev->resources->plane_res->count_planes; ++i)
  {
	  drmModePlane *plane;
	  plane = drmModeGetPlane(dev->fd, dev->resources->plane_res->planes[i]);

	  fprintf(stderr, "\tplane[0x%x]\n", plane->plane_id);
	  fprintf(stderr, "\t\tcrtc_x: %d\n", plane->crtc_x);
	  fprintf(stderr, "\t\tcrtc_y: %d\n", plane->crtc_y);
	  fprintf(stderr, "\t\t     x: %d\n", plane->x);
	  fprintf(stderr, "\t\t     y: %d\n", plane->y);
  }

  return dev;
}


int drm_mode_setup(struct device *dev, uint16_t hdisplay, uint16_t vdisplay)
{
  drmModeModeInfo *mode = NULL;
  struct kms_bo *kms_bo;
  int pitch, bo_handle, fb_id;
  int i;
  int ret = 0;

  for(i=0; i < dev->resources->connector.connector->count_modes; ++i)
  {
    mode = &dev->resources->connector.connector->modes[i];

    if((mode->hdisplay == hdisplay) && (mode->vdisplay == vdisplay))
    {
      break;
    }
  }

  if(!mode)
  {
    return 1;
  }

  if((mode->hdisplay != hdisplay) || (mode->vdisplay != vdisplay))
  {
	fprintf(stderr, "No mode for %dx%d available. Resizing primary plane...\n", hdisplay, vdisplay);


  }
  fprintf(stderr, "Selected mode %dx%d\n", mode->hdisplay, mode->vdisplay);

  create_bo(dev->kms_driver, hdisplay, vdisplay,
    &pitch, &kms_bo, &bo_handle);

  ret = drmModeAddFB(dev->fd, hdisplay, vdisplay, 24, 32, pitch, bo_handle, &fb_id);
  if(ret)
  {
    fprintf(stderr, "drmModeAddFB failed (%ux%u): %s\n",
      hdisplay, vdisplay, strerror(errno));
    return ret;
  }

  mode_set(dev, dev->resources->connector.connector, dev->resources->encoder.encoder, fb_id, mode, dev->resources->plane_res->planes[0], hdisplay, vdisplay);

  return ret;
}


unsigned int drm_get_physical_width_mm(struct device *dev)
{
	return dev->resources->connector.connector->mmWidth;
}


unsigned int drm_get_physical_height_mm(struct device *dev)
{
	return dev->resources->connector.connector->mmHeight;
}


int drm_cursor_create(struct device *dev, uint16_t width, uint16_t height)
{
  struct kms_bo *bo;
  int pitch, handle;
  void *map;
  int ret = 0;

  if(dev->cursor.bo_handle != 0)
  {
    fprintf(stderr, "cursors buffer object already created!\n");
    return 1;
  }

  create_bo(dev->kms_driver, width, height, &pitch, &bo, &handle);

  /* map the bo permanently to user space buffer (dynamic cursor changes...) */
  ret = kms_bo_map(bo, &map);
  if(ret){
    fprintf(stderr, "mapping cursor bo failed: %s\n", strerror(errno));
  }

  dev->cursor.bo_handle = handle;
  dev->cursor.data = (char*) map;
  dev->cursor.crtc_id = dev->resources->encoder.encoder->crtc_id;

  drmModeSetCursor(dev->fd, dev->cursor.crtc_id, handle, width, height);

  return ret;
}


void drm_cursor_move(struct device *dev, uint16_t x, uint16_t y)
{
  // also add offset of primary plane to cursor position
  drmModeMoveCursor(dev->fd, dev->cursor.crtc_id, x + dev->mode.off_x, y + dev->mode.off_y);
}


void drm_cursor_image(struct device *dev, const unsigned char* data, unsigned int size)
{
  memcpy(dev->cursor.data, data, size);
}
