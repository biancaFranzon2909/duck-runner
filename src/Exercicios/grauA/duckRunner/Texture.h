#pragma once
#include <glad/glad.h>
#include <stb_image.h>
#include <string>
#include <iostream>

class Texture {
public:
    GLuint id = 0;
    int width = 0, height = 0;

    bool load(const std::string& path) {
        stbi_set_flip_vertically_on_load(false);
        int ch;
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &ch, 4);
        if (!data) {
            std::cerr << "Falha ao carregar textura: " << path << "\n";
            return false;
        }
        upload(data, width, height);
        stbi_image_free(data);
        return true;
    }

    // Carrega pixels RGBA diretamente da memória (para sprites gerados)
    void loadRaw(const unsigned char* data, int w, int h) {
        width = w;
        height = h;
        upload(data, w, h);
    }

    void bind(int unit = 0) const {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, id);
    }

private:
    void upload(const unsigned char* data, int w, int h) {
        if (id) glDeleteTextures(1, &id);
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }
};
