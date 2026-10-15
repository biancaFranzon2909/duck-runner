# RunGluGlu — Endless Runner 2D em OpenGL

**Vídeo do Jogo:** https://drive.google.com/file/d/1_R69ncHY7Vv9JgKmj2dmr_py1YT86ec7/view?usp=drive_link

## Integrantes do Grupo

* **Bianca da Silva Franzon**

* **Tobias Klein**

* **Bruno Dominick**

## Sobre o Jogo

O RunGluGlu é um jogo do gênero Endless Runner 2D desenvolvido inteiramente em C++ com OpenGL Moderno (Pipeline Programável - GLSL 4.0).

No jogo, o jogador controla um peru que deve correr em um campo infinito e desviar de raposas. Quanto mais corre maior seu score final.

## Arquitetura e Tecnologias Usadas

O projeto foi construído em um único arquivo de código-fonte "RunGluGlu.cpp", focado em performance, legibilidade e facilidade de compilação.

### Bibliotecas Utilizadas:

* **GLFW:** Gerenciamento da janela, contexto gráfico e entradas de teclado.

* **GLAD:** Carregamento das extensões e funções do OpenGL moderno.

* **GLM (OpenGL Mathematics):** Operações matemáticas com vetores e matrizes (Projeção Ortográfica e Matrizes de Transformação).

* **stb_image:** Carregamento de arquivos de imagem .png para texturas.

* **stb_truetype:** Renderização de fontes vetoriais .ttf gerando *texture atlas* dinamicamente.

## Estrutura Técnica e pipeline Gráfico

### 1. Geometria Única (Quad Base)

Toda a interface gráfica (cenários, personagens e interface) utiliza um único retângulo base (2 triângulos, totalizando 6 vértices) gerado em `setupGeometry()`. Cada vértice contém 5 atributos: x, y, z, u, v:

* **Posição 3D x, y, z:** Posição espacial relativa ao quadrado unitário.

* **Coordenadas UV - u, v:** Mapeamento do recorte da textura no quadrado.

### 2. VBO e VAO

* **VBO (Vertex Buffer Object):** Armazena os dados brutos dos vértices diretamente na memória de alta velocidade da placa de vídeo (GPU).

* **VAO (Vertex Array Object):** Guarda as instruções de como a GPU deve interpretar o VBO (onde começam os atributos de posição e onde começam os atributos UV).

### 3. Shaders Dinâmicos (GLSL 4.0)

* **Vertex Shader:** Recebe a matriz de projeção ortográfica e a matriz de modelo model para posicionar e redimensionar os objetos. Utiliza os uniforms `uvOffset` e `uvScale` para realizar recortes dinâmicos de spritesheets sem alterar a geometria.

* **Fragment Shader:** Opera com um modo duplo via flag `textMode`:

  * `textMode = 0`: Processa a textura normal em padrão RGBA dos sprites.

  * `textMode = 1`: Usa o canal vermelho da textura de fonte gerada pelo `stb_truetype` como máscara de transparência (Alpha) para desenhar o texto do placar.

### 4. Animação, Parallax e Colisão

* **Animações:** Controladas por cronômetros em tempo real "dt" que alteram o `uvOffset` na spritesheet.

* **Fundo Parallax:** 4 camadas independentes com rolagem contínua acelerada por `gameSpeed` usando `GL_REPEAT`.

* **Colisão:** Algoritmo AABB (*Axis-Aligned Bounding Box*) que verifica a sobreposição dos eixos X e Y entre o peru e as raposas.

## Como Compilar e Executar

### Pré-requisitos:

* Compilador C++17 (GCC/Clang/MSVC)

* Drivers gráficos com suporte a OpenGL 4.0+

### Exemplo de Compilação (GCC/MinGW):
* Selecione o Kit correto para fazer o Build.
* Dentro da pasta "Build":

```
cmake --build .
```
* Espere executar por completo.
* Execute o seguinte comando.
```
.\RunGluGlu.exe
```