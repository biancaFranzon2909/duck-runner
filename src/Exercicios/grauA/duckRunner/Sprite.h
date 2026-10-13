#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Shader.h"
#include "Texture.h"

// Renderizador de quad compartilhado entre todos os sprites
struct QuadRenderer {
    GLuint vao = 0, vbo = 0;

    void init() {
        // Quad unitário: posição (0..1, 0..1) + UV (0..1, 0..1)
        // Y cresce para baixo (espaço de tela)
        float verts[] = {
            0.f,0.f,  0.f,0.f,
            1.f,0.f,  1.f,0.f,
            1.f,1.f,  1.f,1.f,
            0.f,0.f,  0.f,0.f,
            1.f,1.f,  1.f,1.f,
            0.f,1.f,  0.f,1.f,
        };
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        // location 0: posição (x, y)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        // location 1: UV (u, v)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindVertexArray(0);
    }

    void draw() const {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }
};

// Controla animação de spritesheet horizontal
struct SpriteAnim {
    int frameCount = 1;
    int curFrame   = 0;
    float frameTime = 0.1f;   // segundos por frame
    float timer    = 0.f;
    bool  loop     = true;

    void update(float dt) {
        timer += dt;
        if (timer >= frameTime) {
            timer -= frameTime;
            curFrame = (curFrame + 1) % frameCount;
        }
    }

    void reset() { curFrame = 0; timer = 0.f; }

    void setFrameTime(float ft) { frameTime = ft; }
};

// Desenha um sprite de uma spritesheet horizontal
//  x, y      : canto superior-esquerdo na tela (pixels)
//  w, h      : largura/altura de exibição (pixels)
//  curFrame  : índice do frame atual (0-based)
//  totalFrames: total de frames na spritesheet
//  flipX     : espelhar horizontalmente
//  alpha     : transparência (0..1)
inline void drawSprite(
    Shader& sh, const QuadRenderer& qr, Texture& tex,
    float x, float y, float w, float h,
    int curFrame, int totalFrames,
    bool flipX = false, float alpha = 1.f)
{
    sh.use();
    tex.bind(0);
    sh.setInt("tex", 0);
    sh.setFloat("alpha", alpha);

    float uvW = 1.f / totalFrames;
    float uvX = curFrame * uvW;

    if (flipX) {
        sh.setVec2("uvOffset", uvX + uvW, 0.f);
        sh.setVec2("uvScale",  -uvW, 1.f);
    } else {
        sh.setVec2("uvOffset", uvX, 0.f);
        sh.setVec2("uvScale",   uvW, 1.f);
    }

    glm::mat4 model(1.f);
    model = glm::translate(model, glm::vec3(x, y, 0.f));
    model = glm::scale(model, glm::vec3(w, h, 1.f));
    sh.setMat4("model", glm::value_ptr(model));

    qr.draw();
}

// Desenha fundo com parallax (UV offset + GL_REPEAT)
//  scrollX : deslocamento em pixels do fundo
inline void drawBackground(
    Shader& sh, const QuadRenderer& qr, Texture& tex,
    float scrollX, float winW, float winH,
    float drawX = 0.f, float drawY = 0.f,
    float drawW = -1.f, float drawH = -1.f)
{
    if (drawW < 0) drawW = winW;
    if (drawH < 0) drawH = winH;

    sh.use();
    tex.bind(0);
    sh.setInt("tex", 0);
    sh.setFloat("alpha", 1.f);

    // UV offset = fração do scroll em relação à largura da textura
    float uvOffX = scrollX / (float)tex.width;
    sh.setVec2("uvOffset", uvOffX, 0.f);
    sh.setVec2("uvScale",  drawW / (float)tex.width, 1.f);

    glm::mat4 model(1.f);
    model = glm::translate(model, glm::vec3(drawX, drawY, 0.f));
    model = glm::scale(model, glm::vec3(drawW, drawH, 1.f));
    sh.setMat4("model", glm::value_ptr(model));

    qr.draw();
}
