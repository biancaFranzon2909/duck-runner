/*
 * RunGluGlu - Trabalho Grau A - Processamento Gráfico UNISINOS
 */

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// STB Image para carregar os PNGs
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// STB TrueType para renderização de texto
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

using namespace glm;

// Protótipos
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
int setupShader();
int setupGeometry();
GLuint carregarTextura(string caminho, bool inverterY);
bool checkCollision(vec2 pos1, vec2 size1, vec2 pos2, vec2 size2);
bool initFont(const char* path);
float textWidth(const std::string& text, float fontSize);
void drawText(const std::string& text, float x, float y, float fontSize,
              GLint modelLoc, GLint offsetLoc, GLint scaleLoc,
              GLint textModeLoc, GLint textColorLoc, GLuint VAO);

// Dimensões da janela
const GLuint WIDTH = 800, HEIGHT = 600;

// Variáveis de Física e Jogo
bool isJumping = false;
float playerY = 150.0f;          // Altura base (chão)
float playerVY = 0.0f;
const float GRAVITY = -1800.0f;  // Gravidade
const float JUMP_FORCE = 650.0f; // Força do pulo
const float GROUND_Y = 150.0f;   // Linha do chão

struct Obstacle {
    vec2 position;
    vec2 size;
};

vector<Obstacle> obstacles;
float spawnTimer = 0.0f;
float gameSpeed = 350.0f;

// IDs das Texturas
GLuint texCamadas[4] = {0, 0, 0, 0};
int    largurasCamadas[4] = {0, 0, 0, 0};
float  scrollCamadas[4]   = {0.f, 0.f, 0.f, 0.f};
// 1=céu (lento), 2=montanha (lento), 3=grama (rápido), 4=flores (bem rápido)
const float LAYER_SPEED[4] = {0.02f, 0.08f, 0.35f, 0.90f};
GLuint texPeru = 0;
GLuint texRaposa = 0;
GLuint texOverlay = 0;
GLuint texPlacaGameOver = 0;
GLuint texPlacaPontuacao = 0;
bool gameOver = false;

stbtt_bakedchar fontChars[96]; // ASCII 32–127
GLuint texFont = 0;
float metrosPercorridos = 0.0f;

float animTimer = 0.0f;
int frameAtual = 0;
const int TOTAL_COLS_PERU = 6;
const int TOTAL_ROWS_PERU = 8;

float animTimerRaposa = 0.0f;
int frameAtualRaposa = 0;
const int TOTAL_COLS_RAPOSA = 4;

// Shaders para desenhar com coordenadas UV
const GLchar *vertexShaderSource = R"glsl(
#version 400
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoordIn;
uniform mat4 projection;
uniform mat4 model;
uniform vec2 uvOffset;
uniform vec2 uvScale;
out vec2 texCoordOut;

void main()
{
    gl_Position = projection * model * vec4(position, 1.0);
    texCoordOut = texCoordIn * uvScale + uvOffset;
}
)glsl";

const GLchar *fragmentShaderSource = R"glsl(
#version 400
in vec2 texCoordOut;
uniform sampler2D minhaTextura;
uniform int  textMode;
uniform vec4 textColor;
out vec4 color;

void main()
{
    if (textMode == 1) {
        float a = texture(minhaTextura, texCoordOut).r;
        color = vec4(textColor.rgb, a * textColor.a);
    } else {
        color = texture(minhaTextura, texCoordOut);
    }
}
)glsl";

int main()
{
    srand((unsigned int)time(nullptr));
    glfwInit();

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "RunGluGlu", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Falha ao criar a janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Falha ao inicializar GLAD" << std::endl;
        return -1;
    }

    // Ativa transparência para os PNGs
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint shaderID = setupShader();
    GLuint VAO = setupGeometry();

    glUseProgram(shaderID);

    // Textura 1x1 para o overlay de game over (preto semi-transparente)
    glGenTextures(1, &texOverlay);
    glBindTexture(GL_TEXTURE_2D, texOverlay);
    unsigned char overlayPixel[4] = {0, 0, 0, 180};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, overlayPixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    initFont("../assets/fredoka-one.one-regular.ttf");

    // Carregando as texturas: Fundo e sprites com true para desinverter
    texCamadas[0] = carregarTextura("../assets/background/1.png", true);
    texCamadas[1] = carregarTextura("../assets/background/2.png", true);
    texCamadas[2] = carregarTextura("../assets/background/3.png", true);
    texCamadas[3] = carregarTextura("../assets/background/4.png", true);
    // Guardar larguras para o cálculo do UV offset no loop
    for (int i = 0; i < 4; i++) {
        glBindTexture(GL_TEXTURE_2D, texCamadas[i]);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &largurasCamadas[i]);
    }
    texPeru = carregarTextura("../assets/peru.png", true);
    texRaposa = carregarTextura("../assets/raposa.png", true);
    texPlacaGameOver  = carregarTextura("../assets/background/placaGameOver.png", true);
    texPlacaPontuacao = carregarTextura("../assets/background/placaPontuacao.png", true);

    mat4 projection = ortho(0.0f, 800.0f, 0.0f, 600.0f, -1.0f, 1.0f);
    GLint projLoc = glGetUniformLocation(shaderID, "projection");
    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint texLoc = glGetUniformLocation(shaderID, "minhaTextura");
    GLint offsetLoc = glGetUniformLocation(shaderID, "uvOffset");
    GLint scaleLoc = glGetUniformLocation(shaderID, "uvScale");
    GLint textModeLoc  = glGetUniformLocation(shaderID, "textMode");
    GLint textColorLoc = glGetUniformLocation(shaderID, "textColor");

    glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(projection));
    glUniform1i(texLoc, 0);
    glUniform1i(textModeLoc, 0);

    double prev_s = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        double curr_s = glfwGetTime();
        float dt = (float)(curr_s - prev_s);
        prev_s = curr_s;
        if (dt > 0.05f) dt = 0.05f;

        glfwPollEvents();

        animTimer += dt;
        if (animTimer >= 0.15f) {
            animTimer = 0.0f;
            frameAtual = (frameAtual + 1) % 4;
        }

        animTimerRaposa += dt;
        if (animTimerRaposa >= 0.12f) {
            animTimerRaposa = 0.0f;
            frameAtualRaposa = (frameAtualRaposa + 1) % TOTAL_COLS_RAPOSA;
        }

        // --- SCROLL DO FUNDO (parallax) ---
        if (!gameOver) {
        metrosPercorridos += gameSpeed * dt / 200.0f;
        for (int i = 0; i < 4; i++)
            scrollCamadas[i] += gameSpeed * LAYER_SPEED[i] * dt;

        // --- LÓGICA DO PULO ---
        if (isJumping) {
            playerVY += GRAVITY * dt;
            playerY += playerVY * dt;

            if (playerY <= GROUND_Y) {
                playerY = GROUND_Y;
                isJumping = false;
                playerVY = 0.0f;
            }
        }

        // --- SPAWN DE OBSTÁCULOS ---
        spawnTimer -= dt;
        if (spawnTimer <= 0.0f) {
            spawnTimer = (rand() % 15 + 15) / 10.0f;
            Obstacle obs;
            obs.position = vec2(850.0f, GROUND_Y);
            obs.size = vec2(45.0f, 45.0f);
            obstacles.push_back(obs);
        }

        // --- MOVIMENTO E COLISÃO ---
        for (auto &obs : obstacles) {
            obs.position.x -= gameSpeed * dt;

            if (checkCollision(vec2(150.0f, playerY), vec2(40.0f, 40.0f), obs.position, obs.size)) {
                playerY = GROUND_Y;
                isJumping = false;
                playerVY = 0.0f;
                obstacles.clear();
                gameOver = true;
                break;
            }
        }

        obstacles.erase(
            remove_if(obstacles.begin(), obstacles.end(), [](const Obstacle& o) { return o.position.x + o.size.x < 0; }),
            obstacles.end()
        );
        } // fim if (!gameOver)

        // --- RENDERIZAÇÃO ---
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        glClearColor(0.15f, 0.15f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(VAO);
        glActiveTexture(GL_TEXTURE0);

        // 1. Desenhar as camadas de fundo (parallax)
        mat4 model = mat4(1.0f);
        model = scale(model, vec3(800.0f, 600.0f, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));
        for (int i = 0; i < 4; i++) {
            glBindTexture(GL_TEXTURE_2D, texCamadas[i]);
            float uvOff = scrollCamadas[i] / (float)largurasCamadas[i];
            glUniform2f(offsetLoc, uvOff, 0.0f);
            glUniform2f(scaleLoc, 800.0f / (float)largurasCamadas[i], 1.0f);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // 2. Desenhar o Jogador (Peru)
        glBindTexture(GL_TEXTURE_2D, texPeru);
        float uWidth = 1.0f / (float)TOTAL_COLS_PERU;
        float vHeight = 1.0f / (float)TOTAL_ROWS_PERU;
        float uOffset = frameAtual * uWidth;
        float vOffset = 4.0f * vHeight; 

        glUniform2f(offsetLoc, uOffset, vOffset);
        glUniform2f(scaleLoc, uWidth, vHeight);

        model = mat4(1.0f);
        model = translate(model, vec3(150.0f, playerY, 0.0f));
        model = scale(model, vec3(80.0f, 80.0f, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // 3. Desenhar os Obstáculos (Raposa)
        glBindTexture(GL_TEXTURE_2D, texRaposa);
        glUniform2f(offsetLoc, frameAtualRaposa * (1.0f / TOTAL_COLS_RAPOSA), 5.0f * (1.0f / 4.0f));
        glUniform2f(scaleLoc, 1.0f / TOTAL_COLS_RAPOSA, 0.25f);

        for (const auto &obs : obstacles) {
            model = mat4(1.0f);
            model = translate(model, vec3(obs.position.x, obs.position.y, 0.0f));
            model = scale(model, vec3(60.0f, 60.0f, 1.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // 4. Overlay de game over
        if (gameOver) {
            // Overlay escuro cobrindo tudo
            glBindTexture(GL_TEXTURE_2D, texOverlay);
            glUniform2f(offsetLoc, 0.0f, 0.0f);
            glUniform2f(scaleLoc, 1.0f, 1.0f);
            model = mat4(1.0f);
            model = scale(model, vec3(800.0f, 600.0f, 1.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Placa Game Over centralizada (500x334 mantém proporção da imagem)
            const float goW = 500.0f, goH = 334.0f;
            glBindTexture(GL_TEXTURE_2D, texPlacaGameOver);
            glUniform2f(offsetLoc, 0.0f, 0.0f);
            glUniform2f(scaleLoc, 1.0f, 1.0f);
            model = mat4(1.0f);
            model = translate(model, vec3((800.0f - goW) / 2.0f, (600.0f - goH) / 2.0f, 0.0f));
            model = scale(model, vec3(goW, goH, 1.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // 5. Placa de pontuação (canto superior esquerdo, sempre visível)
        {
            const float pW = 320.0f, pH = 240.0f;
            glBindTexture(GL_TEXTURE_2D, texPlacaPontuacao);
            glUniform2f(offsetLoc, 0.0f, 0.0f);
            glUniform2f(scaleLoc, 1.0f, 1.0f);
            model = mat4(1.0f);
            model = translate(model, vec3(0.0f, 600.0f - pH, 0.0f));
            model = scale(model, vec3(pW, pH, 1.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Texto de pontuação centralizado na placa
            std::string scoreText = std::to_string((int)metrosPercorridos) + "m";
            const float boardCenterX = 93.0f;
            const float boardBaselineY = 535.0f;
            float tw = textWidth(scoreText, 1.0f);
            drawText(scoreText, boardCenterX - tw / 2.0f, boardBaselineY, 1.0f,
                     modelLoc, offsetLoc, scaleLoc, textModeLoc, textColorLoc, VAO);
        }

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if ((key == GLFW_KEY_SPACE || key == GLFW_KEY_UP) && action == GLFW_PRESS)
    {
        if (gameOver) {
            gameOver = false;
            playerY = GROUND_Y;
            isJumping = false;
            playerVY = 0.0f;
            obstacles.clear();
            spawnTimer = 0.0f;
            metrosPercorridos = 0.0f;
            glfwSetWindowTitle(window, "RunGluGlu");
        } else if (!isJumping) {
            playerVY = JUMP_FORCE;
            isJumping = true;
        }
    }
}

bool checkCollision(vec2 pos1, vec2 size1, vec2 pos2, vec2 size2) {
    return pos1.x < pos2.x + size2.x && pos1.x + size1.x > pos2.x &&
           pos1.y < pos2.y + size2.y && pos1.y + size1.y > pos2.y;
}

int setupShader()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

int setupGeometry()
{
    GLfloat vertices[] = {
        0.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        
        0.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        0.0f, 1.0f, 0.0f,  0.0f, 1.0f
    };

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}

GLuint carregarTextura(string caminho, bool inverterY)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int largura, altura, canais;
    stbi_set_flip_vertically_on_load(inverterY); 
    unsigned char *dados = stbi_load(caminho.c_str(), &largura, &altura, &canais, 0);

    if (dados)
    {
        GLenum formato = GL_RGB;
        if (canais == 4)
            formato = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, formato, largura, altura, 0, formato, GL_UNSIGNED_BYTE, dados);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        stbi_image_free(dados);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    else
    {
        std::cout << "Nao foi possivel carregar a textura: " << caminho << std::endl;
        stbi_image_free(dados);
        return 0;
    }

    return texID;
}

// =============================================================================
//  FONTE / TEXTO
// =============================================================================
bool initFont(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) { std::cerr << "Fonte nao encontrada: " << path << "\n"; return false; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char* buf = new unsigned char[sz];
    fread(buf, 1, sz, f);
    fclose(f);

    const int ATLAS_W = 512, ATLAS_H = 512;
    unsigned char* bitmap = new unsigned char[ATLAS_W * ATLAS_H];
    stbtt_BakeFontBitmap(buf, 0, 28.0f, bitmap, ATLAS_W, ATLAS_H, 32, 96, fontChars);

    glGenTextures(1, &texFont);
    glBindTexture(GL_TEXTURE_2D, texFont);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ATLAS_W, ATLAS_H, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    delete[] buf;
    delete[] bitmap;
    return true;
}

float textWidth(const std::string& text, float fontSize) {
    float w = 0.f;
    for (char c : text) {
        if (c < 32 || c >= 128) continue;
        w += fontChars[c - 32].xadvance * fontSize;
    }
    return w;
}

void drawText(const std::string& text, float x, float y, float fontSize,
              GLint modelLoc, GLint offsetLoc, GLint scaleLoc,
              GLint textModeLoc, GLint textColorLoc, GLuint VAO) {

    glUniform1i(textModeLoc, 1);
    glUniform4f(textColorLoc, 0.30f, 0.15f, 0.04f, 1.0f); // marrom escuro da madeira
    glBindTexture(GL_TEXTURE_2D, texFont);
    glBindVertexArray(VAO);

    const float ATLAS_W = 512.f, ATLAS_H = 512.f;
    float cx = x;

    for (char c : text) {
        if (c < 32 || c >= 128) continue;
        stbtt_bakedchar* bc = &fontChars[c - 32];

        float qx = cx + bc->xoff * fontSize;
        // Converte Y de screen-down para OpenGL Y-up
        float qy = y - bc->yoff * fontSize - (bc->y1 - bc->y0) * fontSize;
        float qw = (bc->x1 - bc->x0) * fontSize;
        float qh = (bc->y1 - bc->y0) * fontSize;

        // uvScale.y negativo inverte a textura verticalmente
        glUniform2f(offsetLoc, bc->x0 / ATLAS_W,  bc->y1 / ATLAS_H);
        glUniform2f(scaleLoc,  (bc->x1 - bc->x0) / ATLAS_W, -(bc->y1 - bc->y0) / ATLAS_H);

        mat4 model = mat4(1.0f);
        model = translate(model, vec3(qx, qy, 0.f));
        model = glm::scale(model, vec3(qw, qh, 1.f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 6);

        cx += bc->xadvance * fontSize;
    }

    glUniform1i(textModeLoc, 0); // restaura modo sprite
}