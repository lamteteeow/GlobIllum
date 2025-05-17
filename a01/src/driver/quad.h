#pragma once

#include <GL/glew.h>
#include <GL/gl.h>

#include "gi/framebuffer.h"

// ------------------------------------------
// Quad definition

/**
 * @brief Full screen quad for simple blitting of OpenGL textures on screen
 */
class Quad {
public:
    Quad(const Framebuffer& fbo);
    ~Quad();

    void draw(const Framebuffer& fbo, float exposure = 1.f) const;
    void resize(const Framebuffer& fbo);

private:
    // data
    GLuint vao, vbo, ibo, shader;

    GLuint gl_tex;                      ///< OpenGL texture
    GLuint gl_buf;                      ///< OpenGL buffer
};
