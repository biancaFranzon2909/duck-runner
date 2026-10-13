#pragma once
// Gera spritesheets e fundos em pixel art diretamente na memória.
// Salva em disco na primeira execução; nas subsequentes, carrega do disco.
#include <vector>
#include <string>
#include <cmath>
#include <stb_image_write.h>
struct Col { uint8_t r, g, b, a; };

// Paleta de cores
static constexpr Col TRANSP     = {  0,   0,   0,   0};
static constexpr Col DUCK_YEL   = {255, 220,   0, 255};
static constexpr Col DUCK_DYEL  = {200, 170,   0, 255};
static constexpr Col DUCK_WING  = {255, 240, 120, 255};
static constexpr Col DUCK_BILL  = {255, 140,   0, 255};
static constexpr Col DUCK_EYE   = { 20,  20,  20, 255};
static constexpr Col DUCK_WHITE = {255, 255, 255, 255};
static constexpr Col DUCK_BROWN = {140,  90,  40, 255};

static constexpr Col CACTUS_G   = { 50, 160,  40, 255};
static constexpr Col CACTUS_DG  = { 30, 110,  20, 255};

static constexpr Col COIN_GOLD  = {255, 215,   0, 255};
static constexpr Col COIN_DGOLD = {200, 160,   0, 255};
static constexpr Col COIN_SHINE = {255, 245, 160, 255};

using Img = std::vector<uint8_t>;

static Img makeImg(int w, int h) { return Img(w * h * 4, 0); }

static void px(Img& img, int iw, int ih, int x, int y, Col c) {
    if (x < 0 || x >= iw || y < 0 || y >= ih) return;
    int i = (y * iw + x) * 4;
    img[i]=c.r; img[i+1]=c.g; img[i+2]=c.b; img[i+3]=c.a;
}

static void fillRect(Img& img, int iw, int ih, int x0, int y0, int x1, int y1, Col c) {
    for (int y=y0; y<=y1; y++) for (int x=x0; x<=x1; x++) px(img,iw,ih,x,y,c);
}

static void fillCircle(Img& img, int iw, int ih, int cx, int cy, int r, Col c) {
    for (int y=cy-r; y<=cy+r; y++)
        for (int x=cx-r; x<=cx+r; x++)
            if ((x-cx)*(x-cx)+(y-cy)*(y-cy) <= r*r)
                px(img,iw,ih,x,y,c);
}

static void fillEllipse(Img& img, int iw, int ih, int cx, int cy, int rx, int ry, Col c) {
    for (int y=cy-ry; y<=cy+ry; y++)
        for (int x=cx-rx; x<=cx+rx; x++) {
            float dx=(float)(x-cx)/rx, dy=(float)(y-cy)/ry;
            if (dx*dx+dy*dy<=1.f) px(img,iw,ih,x,y,c);
        }
}

static void gradient(Img& img, int iw, int ih, Col top, Col bot) {
    for (int y=0; y<ih; y++) {
        float t = (float)y/(ih-1);
        Col c;
        c.r = (uint8_t)(top.r + t*(bot.r-top.r));
        c.g = (uint8_t)(top.g + t*(bot.g-top.g));
        c.b = (uint8_t)(top.b + t*(bot.b-top.b));
        c.a = 255;
        for (int x=0; x<iw; x++) px(img,iw,ih,x,y,c);
    }
}

// ── Duck sprite (64x64, 5 frames) ───────────────────────────────────────────

static void drawDuckBody(Img& img, int iw, int ih, int ox) {
    // Cauda
    fillEllipse(img,iw,ih, ox+10,44, 8,5, DUCK_DYEL);
    // Corpo
    fillEllipse(img,iw,ih, ox+28,42, 20,13, DUCK_YEL);
    // Asa (detalhe claro)
    fillEllipse(img,iw,ih, ox+26,40, 11,7, DUCK_WING);
    // Cabeça
    fillCircle(img,iw,ih, ox+44,28, 13, DUCK_YEL);
    // Bico
    fillRect(img,iw,ih, ox+55,25, ox+63,29, DUCK_BILL);
    fillRect(img,iw,ih, ox+54,27, ox+63,31, DUCK_BILL);
    // Olho
    fillCircle(img,iw,ih, ox+47,24, 3, DUCK_EYE);
    px(img,iw,ih, ox+48,23, DUCK_WHITE); // brilho
}

static void drawDuckLegs(Img& img, int iw, int ih, int ox, int frame) {
    // frame 0,2: perna esquerda baixa, direita alta
    // frame 1,3: perna esquerda alta, direita baixa
    // frame 4  : pernas juntas (pulo)
    int lyL, lyR;
    if (frame == 4) { lyL = lyR = 52; }
    else if (frame % 2 == 0) { lyL=54; lyR=50; }
    else                     { lyL=50; lyR=54; }

    // Perna esquerda
    fillRect(img,iw,ih, ox+21,lyL, ox+26,lyL+8, DUCK_BROWN);
    fillRect(img,iw,ih, ox+18,lyL+7, ox+27,lyL+10, DUCK_BILL);
    // Perna direita
    fillRect(img,iw,ih, ox+31,lyR, ox+36,lyR+8, DUCK_BROWN);
    fillRect(img,iw,ih, ox+28,lyR+7, ox+37,lyR+10, DUCK_BILL);
}

static Img genDuckSheet() {
    const int FW=64, FH=64, FRAMES=5;
    Img img = makeImg(FW*FRAMES, FH);
    for (int f=0; f<FRAMES; f++) {
        drawDuckBody(img, FW*FRAMES, FH, f*FW);
        drawDuckLegs(img, FW*FRAMES, FH, f*FW, f);
    }
    return img;
}

// ── Cactus sprite (64x96, 2 frames) ─────────────────────────────────────────

static void drawCactusFrame(Img& img, int iw, int ih, int ox, bool large) {
    int base = large ? 14 : 24;
    // Tronco principal
    fillRect(img,iw,ih, ox+28,base, ox+36,ih-1, CACTUS_G);
    // Braço esquerdo
    fillRect(img,iw,ih, ox+12,ih/2-10, ox+28,ih/2-2, CACTUS_G);
    fillRect(img,iw,ih, ox+12,ih/2-22, ox+20,ih/2-2, CACTUS_G);
    // Braço direito
    fillRect(img,iw,ih, ox+36,ih/2-18, ox+52,ih/2-10, CACTUS_G);
    fillRect(img,iw,ih, ox+44,ih/2-30, ox+52,ih/2-10, CACTUS_G);
    if (large) {
        // Segundo braço esquerdo menor
        fillRect(img,iw,ih, ox+16,base+8, ox+28,base+16, CACTUS_G);
    }
    // Escurecer bordas (outline)
    for (int y=0; y<ih; y++) for (int x=0; x<iw; x++) {
        int idx = (y*iw+x)*4;
        if (img[idx+3]<10) continue;
        bool edge = false;
        int nx[]={x-1,x+1,x,x}; int ny[]={y,y,y-1,y+1};
        for (int d=0;d<4;d++) {
            if (nx[d]<0||nx[d]>=iw||ny[d]<0||ny[d]>=ih) { edge=true; break; }
            if (img[(ny[d]*iw+nx[d])*4+3]<10) { edge=true; break; }
        }
        if (edge) {
            img[idx]=CACTUS_DG.r; img[idx+1]=CACTUS_DG.g;
            img[idx+2]=CACTUS_DG.b;
        }
    }
}

static Img genCactusSheet() {
    const int FW=64, FH=96;
    Img img = makeImg(FW*2, FH);
    drawCactusFrame(img, FW*2, FH, 0,   false);
    drawCactusFrame(img, FW*2, FH, FW,  true);
    return img;
}

// ── Coin sprite (32x32, 4 frames) ────────────────────────────────────────────

static Img genCoinSheet() {
    const int FW=32, FH=32, FRAMES=4;
    Img img = makeImg(FW*FRAMES, FH);
    // Elipses simulando rotação de moeda
    int rxs[] = {13, 8, 2, 8};
    for (int f=0; f<FRAMES; f++) {
        int ox = f*FW;
        fillEllipse(img, FW*FRAMES, FH, ox+16,16, rxs[f],13, COIN_GOLD);
        // Brilho lateral
        if (rxs[f]>4) {
            fillEllipse(img, FW*FRAMES, FH, ox+13,12, rxs[f]/3,4, COIN_SHINE);
        }
        // Contorno
        for (int y=0;y<FH;y++) for (int x=ox;x<ox+FW;x++) {
            int idx=(y*FW*FRAMES+x)*4;
            if (img[idx+3]<10) continue;
            bool edge=false;
            int nxs[]={x-1,x+1,x,x}; int nys[]={y,y,y-1,y+1};
            for (int d=0;d<4;d++) {
                if (nxs[d]<ox||nxs[d]>=ox+FW||nys[d]<0||nys[d]>=FH) {edge=true;break;}
                if (img[(nys[d]*FW*FRAMES+nxs[d])*4+3]<10){edge=true;break;}
            }
            if (edge) {
                img[idx]=COIN_DGOLD.r; img[idx+1]=COIN_DGOLD.g;
                img[idx+2]=COIN_DGOLD.b;
            }
        }
    }
    return img;
}

// ── Fundos ───────────────────────────────────────────────────────────────────

static Img genBgSky(int w, int h) {
    Img img = makeImg(w,h);
    gradient(img, w, h, {100,160,240,255}, {180,215,255,255});
    // Nuvens em posições fixas
    struct Cloud{ int cx,cy,rx,ry; };
    Cloud clouds[] = {{120,80,50,22},{380,60,65,28},{630,100,45,20},
                      {250,130,35,16},{510,40,40,18},{750,90,30,14}};
    for (auto& c : clouds) {
        fillEllipse(img,w,h, c.cx,   c.cy,   c.rx,   c.ry,   {255,255,255,210});
        fillEllipse(img,w,h, c.cx-25,c.cy+5, c.rx/2, c.ry,   {255,255,255,200});
        fillEllipse(img,w,h, c.cx+25,c.cy+5, c.rx/2, c.ry,   {255,255,255,200});
    }
    return img;
}

static Img genBgMountains(int w, int h) {
    // Fundo transparente com silhueta de montanhas
    Img img = makeImg(w,h);
    gradient(img, w, h, {100,160,240,255}, {180,215,255,255});
    // Montanhas: série de triângulos
    struct Peak{ int px,py; };
    Peak peaks[] = {{0,h},{80,280},{200,230},{350,260},{480,210},{600,250},{720,220},{800,h}};
    int np = 8;
    Col mtnCol = {100,110,130,255};
    Col mtnCol2 = {120,130,150,255};
    for (int y=0;y<h;y++) {
        for (int x=0;x<w;x++) {
            // Ponto está abaixo do topo das montanhas?
            // Interpolar a altura das montanhas em x
            float minH = (float)h;
            for (int i=0;i<np-1;i++) {
                if (x >= peaks[i].px && x <= peaks[i+1].px) {
                    float t = (float)(x-peaks[i].px)/(peaks[i+1].px-peaks[i].px);
                    minH = peaks[i].py + t*(peaks[i+1].py - peaks[i].py);
                }
            }
            if (y >= (int)minH) {
                // Variação de cor por altura
                float shade = (float)(y-(int)minH)/(h-(int)minH);
                Col c;
                c.r = (uint8_t)(mtnCol.r + shade*(mtnCol2.r-mtnCol.r));
                c.g = (uint8_t)(mtnCol.g + shade*(mtnCol2.g-mtnCol.g));
                c.b = (uint8_t)(mtnCol.b + shade*(mtnCol2.b-mtnCol.b));
                c.a = 255;
                px(img,w,h,x,y,c);
            }
        }
    }
    return img;
}

static Img genBgTrees(int w, int h) {
    Img img = makeImg(w,h);
    gradient(img, w, h, {100,160,240,255}, {180,215,255,255});
    // Troncos de árvore
    struct Tree{ int tx,ty,tr; };
    Tree trees[] = {{60,h,18},{160,h,22},{280,h,16},{390,h,20},{500,h,18},{620,h,24},{740,h,17}};
    for (auto& t : trees) {
        // Copa: três círculos sobrepostos
        fillCircle(img,w,h, t.tx,   t.ty-80, t.tr+8, {30,120,30,255});
        fillCircle(img,w,h, t.tx-15,t.ty-65, t.tr+4, {35,130,25,255});
        fillCircle(img,w,h, t.tx+15,t.ty-65, t.tr+4, {35,130,25,255});
        fillCircle(img,w,h, t.tx,   t.ty-50, t.tr+6, {40,140,20,255});
        // Tronco
        fillRect(img,w,h, t.tx-5,t.ty-30, t.tx+5,t.ty, {100,70,40,255});
    }
    return img;
}

static Img genBgGround(int w, int h) {
    Img img = makeImg(w,h);
    // Faixa de grama no topo
    for (int y=0; y<10; y++) fillRect(img,w,h, 0,y, w-1,y, {80,180,50,255});
    // Terra
    for (int y=10; y<h; y++) {
        float shade = (float)(y-10)/(h-11);
        Col c;
        c.r = (uint8_t)(160 + shade*40);
        c.g = (uint8_t)(120 + shade*20);
        c.b = (uint8_t)(70  + shade*20);
        c.a = 255;
        for (int x=0; x<w; x++) px(img,w,h,x,y,c);
    }
    // Linha de detalhe no topo da grama
    fillRect(img,w,h, 0,0, w-1,1, {100,200,60,255});
    return img;
}

// ── API pública ───────────────────────────────────────────────────────────────

// Gera todos os assets e salva em disco se não existirem.
// Retorna false se falhar ao salvar.
static bool generateAssetsIfNeeded(const std::string& dir) {
    auto save = [&](const std::string& name, const Img& data, int w, int h) -> bool {
        std::string path = dir + name;
        // stbi_write_png (stride = w*4 bytes)
        return stbi_write_png(path.c_str(), w, h, 4, data.data(), w*4) != 0;
    };

    // Duck sheet: 320x64 (5 frames de 64x64)
    {
        auto img = genDuckSheet();
        if (!save("duck_sheet.png", img, 64*5, 64)) return false;
    }
    // Cactus sheet: 128x96 (2 frames de 64x96)
    {
        auto img = genCactusSheet();
        if (!save("cactus_sheet.png", img, 64*2, 96)) return false;
    }
    // Coin sheet: 128x32 (4 frames de 32x32)
    {
        auto img = genCoinSheet();
        if (!save("coin_sheet.png", img, 32*4, 32)) return false;
    }
    // Fundos
    { auto img = genBgSky(800,600);      if (!save("bg_sky.png",      img,800,600)) return false; }
    { auto img = genBgMountains(800,600); if (!save("bg_mountains.png",img,800,600)) return false; }
    { auto img = genBgTrees(800,600);    if (!save("bg_trees.png",     img,800,600)) return false; }
    { auto img = genBgGround(800,110);   if (!save("bg_ground.png",    img,800,110)) return false; }

    return true;
}
