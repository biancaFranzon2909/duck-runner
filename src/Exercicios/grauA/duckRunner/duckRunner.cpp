/*
 * Jogo Runner - Versão Final com Sprites Corrigidos
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

using namespace glm;

// Protótipos
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
int setupShader();
int setupGeometry();
GLuint carregarTextura(string caminho, bool inverterY);
bool checkCollision(vec2 pos1, vec2 size1, vec2 pos2, vec2 size2);

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
    bool active;
};

vector<Obstacle> obstacles;
float spawnTimer = 0.0f;
float gameSpeed = 350.0f;

// IDs das Texturas
GLuint texFundo = 0;
GLuint texPeru = 0;
GLuint texRaposa = 0;

float animTimer = 0.0f;
int frameAtual = 0;
const int TOTAL_COLS_PERU = 6;
const int TOTAL_ROWS_PERU = 8;

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
out vec4 color;

void main()
{
    color = texture(minhaTextura, texCoordOut);
}
)glsl";

int main()
{
    srand((unsigned int)time(nullptr));
    glfwInit();

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Duck Runner", nullptr, nullptr);
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

    // Carregando as texturas: Fundo e sprites com true para desinverter
    texFundo = carregarTextura("../assets/background/summer5.png", true);
    texPeru = carregarTextura("../assets/peru.png", true);
    texRaposa = carregarTextura("../assets/raposa.png", true);

    mat4 projection = ortho(0.0f, 800.0f, 0.0f, 600.0f, -1.0f, 1.0f);
    GLint projLoc = glGetUniformLocation(shaderID, "projection");
    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint texLoc = glGetUniformLocation(shaderID, "minhaTextura");
    GLint offsetLoc = glGetUniformLocation(shaderID, "uvOffset");
    GLint scaleLoc = glGetUniformLocation(shaderID, "uvScale");

    glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(projection));
    glUniform1i(texLoc, 0);

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
            obs.active = true;
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
                break;
            }
        }

        obstacles.erase(
            remove_if(obstacles.begin(), obstacles.end(), [](const Obstacle& o) { return o.position.x + o.size.x < 0; }),
            obstacles.end()
        );

        // --- RENDERIZAÇÃO ---
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        glClearColor(0.15f, 0.15f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(VAO);
        glActiveTexture(GL_TEXTURE0);

        // 1. Desenhar o Fundo
        glBindTexture(GL_TEXTURE_2D, texFundo);
        glUniform2f(offsetLoc, 0.0f, 0.0f);
        glUniform2f(scaleLoc, 1.0f, 1.0f);
        mat4 model = mat4(1.0f);
        model = translate(model, vec3(0.0f, 0.0f, 0.0f));
        model = scale(model, vec3(800.0f, 600.0f, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 6);

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
        glUniform2f(offsetLoc, 0.0f, 5.0f * (1.0f / 4.0f)); 
        glUniform2f(scaleLoc, 0.25f, 0.25f);

        for (const auto &obs : obstacles) {
            model = mat4(1.0f);
            model = translate(model, vec3(obs.position.x, obs.position.y, 0.0f));
            model = scale(model, vec3(60.0f, 60.0f, 1.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 6);
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
        if (!isJumping)
        {
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