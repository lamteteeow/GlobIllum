#include "quad.h"

#include <string>

// ------------------------------------------
// Quad shader source

static const char *const source_vert_quad = R"glsl(#version 300 es

in vec3 in_pos;
in vec2 in_tc;

out vec2 tc;

void main() {
	tc = in_tc;
	vec4 pos= vec4(vec3(2.0)*in_pos - vec3(1.0), 1.0);
	pos.z = -1.0;
	gl_Position = pos;
}
)glsl";

static const char *const source_frag_quad = R"glsl(#version 300 es
precision mediump float;

in vec2 tc;
out vec4 out_col;

uniform float exposure;
uniform sampler2D in_tex;

float rgb_to_srgb(float val) {
    if (val <= 0.0031308f) return 12.92f * val;
    return 1.055f * pow(val, 1.f / 2.4f) - 0.055f;
}
vec3 rgb_to_srgb(vec3 rgb) {
    return vec3(rgb_to_srgb(rgb.x), rgb_to_srgb(rgb.y), rgb_to_srgb(rgb.z));
}

void main() {
    out_col = exposure * texture(in_tex, tc);
    out_col.rgb = rgb_to_srgb(out_col.rgb);
}
)glsl";

// ------------------------------------------
// Quad implementation

static void createGlTexture(const Framebuffer& fbo, GLuint& gl_buf, GLuint& gl_tex) {
    glViewport(0, 0, fbo.width(), fbo.height());
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glGenBuffers(1, &gl_buf);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gl_buf);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, sizeof(glm::vec3) * fbo.width() * fbo.height(), fbo.data(), GL_DYNAMIC_DRAW);

    glGenTextures(1, &gl_tex);
    glBindTexture(GL_TEXTURE_2D, gl_tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, fbo.width(), fbo.height(), 0, GL_RGB, GL_FLOAT, nullptr);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gl_buf);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, fbo.width(), fbo.height(), GL_RGB, GL_FLOAT, 0);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

Quad::Quad(const Framebuffer& fbo) {
    createGlTexture(fbo, gl_buf, gl_tex);

    // vertices
    static float vertices[20] = {
        0, 0, 1, 0, 0,    // Vertex 0: position (0,0,1), tex coord (0,0)
        1, 0, 1, 1, 0,    // Vertex 1: position (1,0,1), tex coord (1,0)
        1, 1, 1, 1, 1,    // Vertex 2: position (1,1,1), tex coord (1,1)
        0, 1, 1, 0, 1     // Vertex 3: position (0,1,1), tex coord (0,1)
    };

    // indices
    static uint32_t indices[6] = {0, 1, 2, 2, 3, 0};

    // push quad
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, 0);
    glBindVertexArray(0);

    // compile shader, while shamelessly ignoring errors
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &source_vert_quad, NULL);
    glCompileShader(vert);

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &source_frag_quad, NULL);
    glCompileShader(frag);

    shader = glCreateProgram();
    glAttachShader(shader, vert);
    glAttachShader(shader, frag);
    glLinkProgram(shader);
}

Quad::~Quad() {
    glDeleteBuffers(1, &ibo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shader);

    glDeleteBuffers(1, &gl_buf);
    glDeleteTextures(1, &gl_tex);
}

void Quad::resize(const Framebuffer& fbo) {
    glDeleteBuffers(1, &gl_buf);
    glDeleteTextures(1, &gl_tex);

    createGlTexture(fbo, gl_buf, gl_tex);
}

void Quad::draw(const Framebuffer& fbo, float exposure) const {
    // upload fbo data
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gl_buf);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, sizeof(glm::vec3) * fbo.width() * fbo.height(), fbo.data(), GL_DYNAMIC_DRAW);

    glBindTexture(GL_TEXTURE_2D, gl_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, fbo.width(), fbo.height(), GL_RGB, GL_FLOAT, 0);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // draw quad
    glUseProgram(shader);
    glBindTexture(GL_TEXTURE_2D, gl_tex);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    glUniform1i(glGetUniformLocation(shader, "in_tex"), 0);
    glUniform1f(glGetUniformLocation(shader, "exposure"), exposure);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glUseProgram(0);
}
