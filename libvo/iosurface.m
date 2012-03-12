/*
 * IOSurface Common Functions
 *
 * This file is part of mplayer2.
 *
 * mplayer2 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mplayer2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with mplayer2; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "iosurface.h"

#define NUM_IOSURFACE_BUFFERS 2

IOSurfaceRef _iosurface_buffers[NUM_IOSURFACE_BUFFERS];
GLuint _texture_names[NUM_IOSURFACE_BUFFERS];

void init_iosurface_buffers(int width, int heigth)
{
    int i;
  	for(i = 0; i < NUM_IOSURFACE_BUFFERS; i++)
		  _ioSurfaceBuffers[i] = IOSurfaceCreate((CFDictionaryRef)[NSDictionary dictionaryWithObjectsAndKeys:
        [NSNumber numberWithInt:width],  (id)kIOSurfaceWidth,
        [NSNumber numberWithInt:height], (id)kIOSurfaceHeight,
        [NSNumber numberWithInt:4],      (id)kIOSurfaceBytesPerElement,
        [NSNumber numberWithBool:YES],   (id)kIOSurfaceIsGlobal,
        nil]);
}

GLuint setup_iosurface_texture(IOSurfaceRef iosurface_buffer)
{
    GLuint name;
    CGLContextObj cgl_ctx = (CGLContextObj)[[self openGLContext] CGLContextObj];

    glGenTextures(1, &name);

    glBindTexture(GL_TEXTURE_RECTANGLE_EXT, name);
    CGLTexImageIOSurface2D(cgl_ctx, GL_TEXTURE_RECTANGLE_EXT, GL_RGBA, 512, 512, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
            iosurface_buffer, 0);
    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);	

    // Generate an FBO using the same name with the same texture bound to it as a render target.
    glBindTexture(GL_TEXTURE_RECTANGLE_EXT, 0);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, name);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_EXT, name, 0);

    if(!_depthBufferName) {
        glGenRenderbuffersEXT(1, &_depthBufferName);
        glRenderbufferStorageEXT(GL_TEXTURE_RECTANGLE_EXT, GL_DEPTH, 512, 512);
    }
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_RECTANGLE_EXT, _depthBufferName);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    return name;
}

void iosurface_bind_current()
{
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, [[NSApp delegate] currentTextureName]);
}

void iosurface_unbind()
{
   	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}
