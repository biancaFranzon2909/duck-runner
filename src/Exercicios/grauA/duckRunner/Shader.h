#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

class Shader {
public:
    GLuint id = 0;

    void init(const char* vsrc, const char* fsrc) {
        GLuint vs = compile(GL_VERTEX_SHADER, vsrc);
        GLuint fs = compile(GL_FRAGMENT_SHADER, fsrc);
        id = glCreateProgram();
        glAttachShader(id, vs);
        glAttachShader(id, fs);
        glLinkProgram(id);
        GLint ok;
        glGetProgramiv(id, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetProgramInfoLog(id, 512, nullptr, log);
            std::cerr << "Shader link error: " << log << "\n";
        }
        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    void use() const { glUseProgram(id); }

    void setMat4(const char* n, const float* v) const {
        glUniformMatrix4fv(glGetUniformLocation(id, n), 1, GL_FALSE, v);
    }
    void setVec2(const char* n, float x, float y) const {
        glUniform2f(glGetUniformLocation(id, n), x, y);
    }
    void setVec4(const char* n, float x, float y, float z, float w) const {
        glUniform4f(glGetUniformLocation(id, n), x, y, z, w);
    }
    void setInt(const char* n, int v) const {
        glUniform1i(glGetUniformLocation(id, n), v);
    }
    void setFloat(const char* n, float v) const {
        glUniform1f(glGetUniformLocation(id, n), v);
    }

private:
    static GLuint compile(GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetShaderInfoLog(s, 512, nullptr, log);
            std::cerr << "Shader compile error: " << log << "\n";
        }
        return s;
    }
};
