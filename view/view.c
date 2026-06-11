/*
 * view/view.c — Camada VIEW do PAC-MAN
 *
 * Renderizacao com dirty-tracking: apenas celulas alteradas
 * sao redesenhadas, eliminando o piscar do mapa.
 *
 * Personagens inspirados nas imagens de referencia:
 *   PAC-MAN  : C> <C ^C vC () — circulo amarelo com boca direcional
 *   FANTASMA : OO em fundo colorido — olhos brancos grandes caracteristicos
 */

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <unistd.h>
#include <termios.h>
#endif

#include <string.h>
#include "../pacman.h"

#define MAP_COLS (MAP_W * CELL_W)   /* 56 colunas */

/* ════════════════════════════════════════════════════════
   CARACTERES UNICODE (UTF-8) — console em CP_UTF8 (view_init)
   Bytes UTF-8 explicitos: independem da codificacao do .c
   ════════════════════════════════════════════════════════ */
#define SELECTOR   "\xE2\x96\xBA"   /* U+25BA  ►  seta de menu arcade        */
#define PAC_RIGHT  "\xE1\x97\xA7"   /* U+15E7  ᗧ  Pac-Man boca direita  */
#define PAC_LEFT   "\xE1\x97\xA4"   /* U+15E4  ᗤ  Pac-Man boca esquerda */
#define PAC_UP     "\xE1\x97\xA2"   /* U+15E2  ᗢ  Pac-Man boca cima     */
#define PAC_DOWN   "\xE1\x97\xA3"   /* U+15E3  ᗣ  Pac-Man boca baixo    */
#define PAC_CLOSED "\xE2\x97\x8F"   /* U+25CF  ●  Pac-Man boca fechada  */
#define GHOST_CH   "\xE1\x97\xA3"   /* U+15E3  ᗣ  silhueta de fantasma  */
#define DOT_CH     "\xC2\xB7"       /* U+00B7  ·  ponto comivel          */
#define CHERRY      "\xF0\x9F\x8D\x92" /* U+1F352 🍒 cereja (double-width)   */
#define GHOST_EMOJI "\xF0\x9F\x91\xBB" /* U+1F47B 👻 fantasma (double-width) */
#define EYES_EMOJI  "\xF0\x9F\x91\x80" /* U+1F440 👀 olhos (fantasma comido) */
#define HEART_FULL  "\xE2\x9D\xA4"     /* U+2764  ❤  vida cheia              */
#define HEART_EMPTY "\xE2\x99\xA1"     /* U+2661  ♡  vida perdida            */

/* ════════════════════════════════════════════════════════
   CORES VIBRANTES — truecolor 24-bit (\033[38;2;R;G;Bm)
   Suportado pelo Windows Terminal; tons muito mais vivos
   que as 16 cores ANSI basicas.
   ════════════════════════════════════════════════════════ */
#define VIBRANT_YELLOW "\033[38;2;255;235;0m"   /* Pac-Man amarelo vivo   */
#define VIBRANT_RED    "\033[38;2;255;40;40m"   /* vermelho cereja vivo   */

/* Moldura de linha dupla (UTF-8) para paineis estruturados */
#define BOX_TL "\xE2\x95\x94"   /* ╔ */
#define BOX_TR "\xE2\x95\x97"   /* ╗ */
#define BOX_BL "\xE2\x95\x9A"   /* ╚ */
#define BOX_BR "\xE2\x95\x9D"   /* ╝ */
#define BOX_H  "\xE2\x95\x90"   /* ═ */
#define BOX_V  "\xE2\x95\x91"   /* ║ */
#define BOX_ML "\xE2\x95\xA0"   /* ╠ */
#define BOX_MR "\xE2\x95\xA3"   /* ╣ */
#define ARROW_UP "\xE2\x86\x91" /* ↑ */
#define ARROW_DN "\xE2\x86\x93" /* ↓ */

/* Cores do tabuleiro (truecolor) — paleta classica do Pac-Man */
#define WALL_BG "\033[48;2;33;33;222m"    /* parede azul royal #2121DE */
#define DOT_FG  "\033[38;2;255;184;151m"  /* pontos cor pessego        */

/* ════════════════════════════════════════════════════════
   DIRTY-TRACKING — evita redesenho desnecessario do mapa
   ════════════════════════════════════════════════════════ */
static int  s_prev_grid[MAP_H][MAP_W];
static int  s_prev_px  = -1, s_prev_py  = -1;
static int  s_prev_gx[GHOSTS], s_prev_gy[GHOSTS];
static int  s_prev_debug = -1;
static int  s_full_redraw = 1;

/* Chama ao entrar no estado PLAYING para forcar redesenho completo */
void view_reset_dirty(void) {
    s_full_redraw = 1;
    s_prev_px = s_prev_py = -1;
    s_prev_debug = -1;
    for (int i = 0; i < GHOSTS; i++) s_prev_gx[i] = s_prev_gy[i] = -1;
    memset(s_prev_grid, -1, sizeof(s_prev_grid));
}

/* ════════════════════════════════════════════════════════
   PRIMITIVAS DE TERMINAL
   ════════════════════════════════════════════════════════ */
/* TODAS as primitivas usam ANSI (VT habilitado no Windows).
   Posicao e texto passam pelo MESMO buffer do stdio e saem na
   ordem correta no flush de fim de frame — sem dessincronizar
   cursor x texto (que corrompia o desenho com SetConsoleCursorPosition).
   gotoxy NAO faz flush: o frame inteiro e descarregado de uma vez. */
void gotoxy(int x, int y) {
    printf("\033[%d;%dH", y + 1, x + 1);
}

static void hide_cursor(void) { printf("\033[?25l"); fflush(stdout); }
static void show_cursor(void) { printf("\033[?25h"); fflush(stdout); }

static void clear_screen(void) {
    /* Limpa tela + scrollback e vai para o topo (alinha viewport no WT) */
    printf("\033[2J\033[3J\033[H");
    fflush(stdout);
}

/* ════════════════════════════════════════════════════════
   INICIALIZACAO
   ════════════════════════════════════════════════════════ */
void view_init(void) {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    /* UTF-8 (CP 65001) — habilita caracteres Unicode especiais
       (Pac-Man arcade ᗧ ᗤ ᗢ ᗣ, fantasma ᗣ, seta ►, ponto ·) */
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    COORD bufSize = {120, 44};
    SetConsoleScreenBufferSize(hOut, bufSize);
    SMALL_RECT winRect = {0, 0, 119, 43};
    SetConsoleWindowInfo(hOut, TRUE, &winRect);
    SetConsoleTitleA("PAC-MAN em C - ADS 3o Periodo");
#endif
    /* Redimensiona a janela via ANSI (CSI 8 ; linhas ; colunas t).
       O Windows Terminal IGNORA as APIs legadas acima, mas respeita
       esta sequencia — garante 44 linhas (tabuleiro tem 31) e evita
       que o fundo role/seja cortado (bug "parte inferior comida"). */
    printf("\033[8;44;120t");
    fflush(stdout);
    hide_cursor();
    clear_screen();
}

void view_cleanup(void) {
    show_cursor();
    printf(ANSI_RESET);
    gotoxy(0, MAP_H + 3);
    printf("\n");
    fflush(stdout);
}

/* ════════════════════════════════════════════════════════
   ANIMACAO DE TRANSICAO (M04)
   ════════════════════════════════════════════════════════ */
void view_transition(void) {
    for (int row = 0; row < MAP_H; row++) {
        gotoxy(0, row);
        printf("\033[44m");
        for (int i = 0; i < MAP_COLS + 30; i++) printf(" ");
        printf(ANSI_RESET);
        fflush(stdout);
#ifdef _WIN32
        Sleep(14);
#else
        usleep(14000);
#endif
    }
    clear_screen();
    view_reset_dirty();
}

/* ════════════════════════════════════════════════════════
   RENDERIZACAO DE CELULAS (2 chars cada)
   ════════════════════════════════════════════════════════ */
static void print_cell(int cell, int dbg) {
    /* \033[40m garante fundo preto em toda celula vazia/dot/power/door,
       evitando vazamento de cor do pac-man ou fantasmas (bug "comendo tabuleiro") */
    if (dbg >= 0 && dbg < 100 && cell != CELL_WALL) {
        printf("\033[40m" ANSI_GREEN "%2d" ANSI_RESET, dbg);
        return;
    }
    switch (cell) {
        case CELL_WALL:   printf(WALL_BG "  " ANSI_RESET);                   break;
        case CELL_DOT:    printf(ANSI_BG_BLACK " " DOT_FG DOT_CH ANSI_RESET); break;
        case CELL_POWER:  printf("\033[40m" CHERRY "\033[0m");            break;
        case CELL_DOOR:   printf("\033[40m\033[33m--\033[0m");            break;
        default:          printf("\033[40m  \033[0m");                    break;
    }
}

/* ════════════════════════════════════════════════════════
   PAC-MAN — caracteres Unicode arcade (CANADIAN SYLLABICS)
   Glifo redondo com "boca" cuja abertura aponta na direcao
   do movimento — identico ao PAC-MAN classico de terminal:
     ᗧ direita   ᗤ esquerda   ᗢ cima   ᗣ baixo   ● fechado
   Amarelo brilhante sobre fundo preto = silhueta limpa.
   Glifo + espaco preenchem a celula de 2 colunas (CELL_W=2).
   ════════════════════════════════════════════════════════ */
static void draw_pacman(GameModel *m) {
    PacMan *p = &m->pacman;
    gotoxy(p->x * CELL_W, p->y);

    /* Alterna boca aberta/fechada — 4 frames cada */
    int boca_aberta = (m->frame_count % 8) < 4;
    const char *ch;

    if (!boca_aberta || (p->dx == 0 && p->dy == 0)) ch = PAC_CLOSED; /* ● */
    else if (p->dx ==  1) ch = PAC_RIGHT;   /* ᗧ */
    else if (p->dx == -1) ch = PAC_LEFT;    /* ᗤ */
    else if (p->dy == -1) ch = PAC_UP;      /* ᗢ */
    else                  ch = PAC_DOWN;    /* ᗣ */

    /* Amarelo vibrante (truecolor) + negrito sobre fundo preto */
    printf("\033[40m" VIBRANT_YELLOW "\033[1m%s \033[0m", ch);
}

/* ════════════════════════════════════════════════════════
   FANTASMAS — emoji 👻 (U+1F47B), formato exato de fantasma
   arredondado com olhos, identico a imagem de referencia.
   Cada fantasma recebe um FUNDO de cor distinta para que seja
   possivel diferencia-los (o glifo do emoji e branco fixo).
   Modos:
     Normal     → 👻 sobre fundo na cor (vermelho/rosa/ciano/laranja)
     Vulneravel → 👻 sobre fundo AZUL (= imagem); pisca quando acaba
     Comido     → 👀 apenas os olhos voltando pra casa
   ════════════════════════════════════════════════════════ */

/* Cor de fundo (corpo) de cada fantasma — usada aqui e no painel */
static const char *s_ghost_bg[GHOSTS] = {
    "\033[41m",    /* Blinky — fundo vermelho       */
    "\033[105m",   /* Pinky  — fundo rosa brilhante */
    "\033[46m",    /* Inky   — fundo ciano          */
    "\033[43m",    /* Clyde  — fundo laranja        */
};

static void draw_ghosts(GameModel *m) {
    for (int i = 0; i < GHOSTS; i++) {
        Ghost *g = &m->ghosts[i];
        if (!g->active) continue;
        gotoxy(g->x * CELL_W, g->y);

        if (g->mode == GMODE_FRIGHTENED) {
            /* Vulneravel: 👻 sobre fundo AZUL (igual a imagem) */
            int pisca = (g->frighten_timer <= 10) && ((m->frame_count % 6) < 3);
            if (pisca) printf("\033[107m" GHOST_EMOJI "\033[0m");  /* pisca: fundo branco */
            else       printf("\033[44m"  GHOST_EMOJI "\033[0m");  /* fundo azul          */
        } else if (g->mode == GMODE_EATEN) {
            /* Comido: apenas os olhos (👀) voltando pra casa */
            printf("\033[40m" EYES_EMOJI "\033[0m");
        } else {
            /* Normal: 👻 sobre fundo na cor do fantasma */
            printf("%s" GHOST_EMOJI "\033[0m", s_ghost_bg[i]);
        }
    }
}

/* ════════════════════════════════════════════════════════
   RENDERIZACAO INTELIGENTE DO MAPA (sem piscar)
   Algoritmo dirty-tracking:
     1. Redesenho completo apenas na 1a vez ou ao trocar modo
     2. Apaga posicao antiga da entidade (so se ela MOVEU)
     3. Redesenha apenas celulas que mudaram (ponto comido etc.)
     4. Desenha entidades nas posicoes atuais
   Com isso cada frame faz apenas ~10 escritas ao inves de 868.
   ════════════════════════════════════════════════════════ */
/* ════════════════════════════════════════════════════════
   RENDERIZACAO INTELIGENTE DO MAPA — dirty-tracking por LINHA

   Estrategia row-level (mais robusta que cell-level):
     · Marca uma linha como "suja" se qualquer entidade estava
       ou esta nela, OU se alguma celula da linha mudou.
     · Redesenha a linha inteira de uma vez: gotoxy(0,y) +
       28 celulas sequenciais, sem saltos — elimina artefatos
       de cor ANSI residual (bug "comendo tabuleiro").
     · Apos todas as linhas, desenha entidades por cima.
   Performance: ~4-6 linhas/frame, 56 chars cada — rapido.
   ════════════════════════════════════════════════════════ */
/* Desenha 1 celula posicionando o cursor de forma ABSOLUTA.
   O gotoxy por celula evita que emojis de largura dupla (🍒/👻)
   desloquem o restante da linha (bug "comendo o tabuleiro"). */
static void draw_cell_at(GameModel *m, int x, int y) {
    int dbg = -1;
    if (m->debug_mode) {
        int v = coord_to_vertex(x, y);
        if (m->dijkstra_dist[v] < 100) dbg = m->dijkstra_dist[v];
    }
    gotoxy(x * CELL_W, y);
    print_cell(m->grid[y][x], dbg);
    s_prev_grid[y][x] = m->grid[y][x];
}

static void draw_map_smart(GameModel *m) {
    int force = s_full_redraw || (s_prev_debug != m->debug_mode);

    if (force) {
        /* Redesenho completo da primeira vez ou troca de modo debug */
        for (int y = 0; y < MAP_H; y++)
            for (int x = 0; x < MAP_W; x++)
                draw_cell_at(m, x, y);
        s_full_redraw = 0;
        s_prev_debug  = m->debug_mode;
    } else {
        /* Dirty-tracking por linha — redesenha linha inteira se necessario */
        for (int y = 0; y < MAP_H; y++) {
            int row_dirty = 0;

            /* Entidade atual ou anterior nesta linha? */
            if (m->pacman.y == y || s_prev_py == y) row_dirty = 1;
            for (int i = 0; i < GHOSTS && !row_dirty; i++) {
                if (!m->ghosts[i].active) continue;
                if (m->ghosts[i].y == y || s_prev_gy[i] == y) row_dirty = 1;
            }

            /* Alguma celula mudou nesta linha? (ex: ponto comido) */
            for (int x = 0; x < MAP_W && !row_dirty; x++) {
                if (m->grid[y][x] != s_prev_grid[y][x]) row_dirty = 1;
            }

            /* Redesenha a linha inteira com gotoxy por celula (sem deslize) */
            if (row_dirty)
                for (int x = 0; x < MAP_W; x++)
                    draw_cell_at(m, x, y);
        }
    }

    /* Entidades por cima do mapa (animacao muda todo frame) */
    draw_pacman(m);
    draw_ghosts(m);

    /* Atualiza rastreamento de posicoes para proximo frame */
    s_prev_px = m->pacman.x;
    s_prev_py = m->pacman.y;
    for (int i = 0; i < GHOSTS; i++) {
        s_prev_gx[i] = m->ghosts[i].x;
        s_prev_gy[i] = m->ghosts[i].y;
    }
}

/* ════════════════════════════════════════════════════════
   PAINEL LATERAL (HUD) — emoldurado, atualiza a cada 3 frames
   ════════════════════════════════════════════════════════ */

/* Imprime n caracteres de borda horizontal (linha dupla) */
static void box_h(int n) { for (int i = 0; i < n; i++) printf(BOX_H); }

/* Borda horizontal do painel: canto/juncao esq + linha + dir */
static void pframe(int px, int row, const char *left, int inw, const char *right) {
    gotoxy(px, row);
    printf(ANSI_BBLUE ANSI_BOLD "%s", left);
    box_h(inw);
    printf("%s" ANSI_RESET, right);
}

/* Linha lateral vazia (║ + fundo preto + ║) — conteudo vai por cima */
static void prow(int px, int row, int inw) {
    gotoxy(px, row);
    printf(ANSI_BBLUE ANSI_BOLD BOX_V ANSI_RESET ANSI_BG_BLACK "%*s" ANSI_RESET
           ANSI_BBLUE ANSI_BOLD BOX_V ANSI_RESET, inw, "");
}

static void draw_panel(GameModel *m) {
    const int px = PANEL_X, INW = 25;
    int cx = px + 2, r = 0;
    const char *spd =
        m->game_speed_ms <= SPEED_EXTREME ? "PRO" :
        m->game_speed_ms <= SPEED_FAST    ? "MEDIO" : "NORMAL";

    /* ── Cabecalho ── */
    pframe(px, r++, BOX_TL, INW, BOX_TR);
    prow(px, r, INW); gotoxy(cx, r++);
    printf(ANSI_BG_BLACK VIBRANT_YELLOW ANSI_BOLD "   P A C - M A N" ANSI_RESET);

    /* ── Status ── */
    pframe(px, r++, BOX_ML, INW, BOX_MR);
    prow(px, r, INW); gotoxy(cx, r++);
    printf(ANSI_BG_BLACK ANSI_BWHITE "SCORE" ANSI_RESET ANSI_BG_BLACK "   "
           ANSI_BYELLOW ANSI_BOLD "%9d" ANSI_RESET, m->score);
    prow(px, r, INW); gotoxy(cx, r++);
    printf(ANSI_BG_BLACK ANSI_BWHITE "NIVEL" ANSI_RESET ANSI_BG_BLACK " "
           ANSI_BYELLOW "%-3d" ANSI_RESET ANSI_BG_BLACK ANSI_DIM "  vel " ANSI_RESET
           ANSI_BG_BLACK ANSI_BCYAN "%-6s" ANSI_RESET, m->level, spd);
    prow(px, r, INW); gotoxy(cx, r++);
    printf(ANSI_BG_BLACK ANSI_BWHITE "VIDAS" ANSI_RESET ANSI_BG_BLACK "  ");
    for (int i = 0; i < m->pacman.lives; i++) printf("\033[91m" ANSI_BOLD HEART_FULL " ");
    for (int i = m->pacman.lives; i < 3; i++) printf(ANSI_DIM HEART_EMPTY " ");
    printf(ANSI_RESET);
    prow(px, r, INW); gotoxy(cx, r++);
    printf(ANSI_BG_BLACK ANSI_BWHITE "PONTOS" ANSI_RESET ANSI_BG_BLACK " "
           ANSI_WHITE "%d / %d" ANSI_RESET, m->stats.total_dots - m->dot_count, m->stats.total_dots);

    /* ── BST ── */
    pframe(px, r++, BOX_ML, INW, BOX_MR);
    prow(px, r, INW); gotoxy(cx, r++);
    printf(ANSI_BG_BLACK ANSI_BCYAN ANSI_BOLD "BST " DOT_CH " HIGH SCORES" ANSI_RESET);
    prow(px, r, INW); gotoxy(cx, r++);
    printf(ANSI_BG_BLACK ANSI_DIM "Altura " ANSI_RESET ANSI_BG_BLACK ANSI_BYELLOW "%-3d" ANSI_RESET
           ANSI_BG_BLACK ANSI_DIM " Nos " ANSI_RESET ANSI_BG_BLACK ANSI_BYELLOW "%-4d" ANSI_RESET,
           m->stats.bst_height, m->stats.bst_count);
    prow(px, r, INW); gotoxy(cx, r++);
    printf(ANSI_BG_BLACK ANSI_DIM "Min " ANSI_RESET ANSI_BG_BLACK ANSI_BYELLOW "%-6d" ANSI_RESET
           ANSI_BG_BLACK ANSI_DIM "Max " ANSI_RESET ANSI_BG_BLACK ANSI_BYELLOW "%-6d" ANSI_RESET,
           m->stats.bst_min_score, m->stats.bst_max_score);

    /* ── AVL ── */
    pframe(px, r++, BOX_ML, INW, BOX_MR);
    prow(px, r, INW); gotoxy(cx, r++);
    printf(ANSI_BG_BLACK ANSI_BMAGENTA ANSI_BOLD "AVL " DOT_CH " POWER-UPS" ANSI_RESET);
    prow(px, r, INW); gotoxy(cx, r++);
    printf(ANSI_BG_BLACK ANSI_DIM "Bal " ANSI_RESET ANSI_BG_BLACK ANSI_BYELLOW "%+d" ANSI_RESET
           ANSI_BG_BLACK ANSI_DIM "  Rot " ANSI_RESET ANSI_BG_BLACK ANSI_BYELLOW "%d" ANSI_RESET
           ANSI_BG_BLACK ANSI_DIM "/" ANSI_RESET ANSI_BG_BLACK ANSI_BYELLOW "%d" ANSI_RESET,
           m->stats.avl_balance, m->stats.avl_rot_simple, m->stats.avl_rot_double);

    /* ── Fantasmas (IA) ── */
    pframe(px, r++, BOX_ML, INW, BOX_MR);
    prow(px, r, INW); gotoxy(cx, r++);
    printf(ANSI_BG_BLACK ANSI_GREEN ANSI_BOLD "FANTASMAS (IA)" ANSI_RESET);
    static const char *ai_lbl[4] = {"Dijkstra","BFS","DFS","Random"};
    for (int i = 0; i < GHOSTS; i++) {
        Ghost *g = &m->ghosts[i];
        prow(px, r, INW); gotoxy(cx, r++);
        printf("%s  " ANSI_RESET ANSI_BG_BLACK " " ANSI_BWHITE "%-7s" ANSI_RESET
               ANSI_BG_BLACK ANSI_DIM "%-8s" ANSI_RESET,
               s_ghost_bg[i], g->name, ai_lbl[g->ai_type]);
    }

    /* ── Controles ── */
    pframe(px, r++, BOX_ML, INW, BOX_MR);
    prow(px, r, INW); gotoxy(cx, r++);
    if (m->debug_mode) printf(ANSI_BG_BLACK ANSI_GREEN "[D] DEBUG ON" ANSI_RESET);
    else               printf(ANSI_BG_BLACK ANSI_DIM "[D]Debug  [P]Pausa" ANSI_RESET);
    prow(px, r, INW); gotoxy(cx, r++);
    printf(ANSI_BG_BLACK ANSI_BYELLOW ANSI_BOLD "[F] FINALIZAR" ANSI_RESET);
    pframe(px, r++, BOX_BL, INW, BOX_BR);
}

/* ════════════════════════════════════════════════════════
   TELA PRINCIPAL DO JOGO
   ════════════════════════════════════════════════════════ */
void view_draw_game(GameModel *m) {
    if (m->state != STATE_PLAYING && m->state != STATE_PAUSED) return;
    draw_map_smart(m);
    /* Painel atualiza a cada 3 frames — nao precisa ser todo frame */
    if (m->frame_count % 3 == 0 || s_prev_debug != m->debug_mode)
        draw_panel(m);
    fflush(stdout);
}

/* ════════════════════════════════════════════════════════
   MENU PRINCIPAL — estetica restaurada com ASCII art
   e cursor de navegacao (setas + ENTER)
   ════════════════════════════════════════════════════════ */
static const char *MENU_ITEMS[5] = {
    "Novo Jogo",
    "Auto-Play  (IA joga so)",
    "Ranking",
    "Ajuda",
    "Sair"
};

/* Posicoes fixas das opcoes DENTRO do quadro do menu */
#define MENU_OPT_COL  18
#define MENU_OPT_ROW  14   /* 1a opcao; seguintes a cada linha */

void view_draw_menu_cursor(GameModel *m) {
    for (int i = 0; i < 5; i++) {
        gotoxy(MENU_OPT_COL, MENU_OPT_ROW + i);
        if (i == m->menu_cursor)
            /* Selecionada: seta amarela + rotulo em destaque (fundo amarelo) */
            printf("\033[40m" ANSI_BYELLOW ANSI_BOLD SELECTOR " "
                   "\033[43m\033[30m\033[1m %-25s \033[0m", MENU_ITEMS[i]);
        else
            /* Normal: texto branco sobre fundo preto */
            printf("\033[40m  " ANSI_BWHITE " %-25s \033[0m", MENU_ITEMS[i]);
    }
    fflush(stdout);
}

void view_draw_menu(GameModel *m) {
    clear_screen();
    const int BX = 14, INW = 36;   /* coluna da borda esquerda e largura interna */
    int ax = 11, r = 1;

    /* ── Logo ASCII art PAC-MAN (amarelo vibrante) ── */
    gotoxy(ax, r++); printf(VIBRANT_YELLOW ANSI_BOLD "  ____   _    ____       __  __    _    _   _ " ANSI_RESET);
    gotoxy(ax, r++); printf(VIBRANT_YELLOW ANSI_BOLD " |  _ \\/ \\  / ___|     |  \\/  |  / \\  | \\ | |" ANSI_RESET);
    gotoxy(ax, r++); printf(VIBRANT_YELLOW ANSI_BOLD " | |_)/ _ \\| |         | |\\/| | / _ \\ |  \\| |" ANSI_RESET);
    gotoxy(ax, r++); printf(VIBRANT_YELLOW ANSI_BOLD " |  _/ ___ \\ |___      | |  | |/ ___ \\| |\\  |" ANSI_RESET);
    gotoxy(ax, r++); printf(VIBRANT_YELLOW ANSI_BOLD " |_|/_/   \\_\\____|     |_|  |_/_/   \\_\\_| \\_|" ANSI_RESET);

    /* ── Subtitulos ── */
    r++;
    gotoxy(16, r++); printf(ANSI_BWHITE "ADS " DOT_CH " 3.o Periodo " DOT_CH " Estruturas de Dados" ANSI_RESET);
    gotoxy(16, r++); printf(ANSI_DIM "BST " DOT_CH " AVL " DOT_CH " Grafos " DOT_CH " 7 Sorts " DOT_CH " BFS/DFS/Dijkstra" ANSI_RESET);

    /* ── Linha decorativa: Pac-Man comendo pontos ── */
    r++;
    gotoxy(20, r++);
    printf(VIBRANT_YELLOW ANSI_BOLD PAC_RIGHT ANSI_RESET ANSI_YELLOW
           "   " DOT_CH "   " DOT_CH "   " DOT_CH "   " DOT_CH "   " DOT_CH "   " DOT_CH "   " DOT_CH ANSI_RESET);

    /* ── Quadro do MENU ── */
    r++;
    int top = r;   /* borda superior; opcoes comecam em top+2 (= MENU_OPT_ROW) */

    /* Borda superior com titulo "MENU" centralizado */
    gotoxy(BX, top);
    printf(ANSI_BBLUE ANSI_BOLD BOX_TL);
    box_h(15);
    printf(ANSI_RESET ANSI_BYELLOW ANSI_BOLD " MENU " ANSI_RESET ANSI_BBLUE ANSI_BOLD);
    box_h(15);
    printf(BOX_TR ANSI_RESET);

    /* Laterais (linhas internas — fundo preto; opcoes desenhadas por cima) */
    for (int row = top + 1; row <= top + 7; row++) {
        gotoxy(BX, row);
        printf(ANSI_BBLUE ANSI_BOLD BOX_V ANSI_RESET "\033[40m%*s" ANSI_RESET
               ANSI_BBLUE ANSI_BOLD BOX_V ANSI_RESET, INW, "");
    }

    /* Borda inferior */
    gotoxy(BX, top + 8);
    printf(ANSI_BBLUE ANSI_BOLD BOX_BL);
    box_h(INW);
    printf(BOX_BR ANSI_RESET);

    /* Opcoes dentro do quadro */
    view_draw_menu_cursor(m);

    /* ── Rodape: instrucoes ── */
    gotoxy(16, top + 10);
    printf(ANSI_BWHITE ARROW_UP ARROW_DN ANSI_DIM " navegar    " ANSI_BWHITE "ENTER"
           ANSI_DIM " selecionar    " ANSI_BWHITE "Q" ANSI_DIM " sair" ANSI_RESET);
    gotoxy(16, top + 11);
    printf(ANSI_DIM "Atalhos: " ANSI_RESET ANSI_WHITE "1" ANSI_DIM " Novo Jogo  " ANSI_RESET
           ANSI_WHITE "2" ANSI_DIM " Ranking  " ANSI_RESET ANSI_WHITE "H" ANSI_DIM " Ajuda" ANSI_RESET);

    fflush(stdout);
}

/* ════════════════════════════════════════════════════════
   SELECAO DE VELOCIDADE
   ════════════════════════════════════════════════════════ */
static const char *SPEED_LABELS[3] = {
    "  Normal  -- 100ms/frame    ",
    "  Medio   --  85ms/frame    ",
    "  Pro     --  50ms/frame    "
};
static const int SPEED_VALUES[3] = {
    SPEED_NORMAL, SPEED_FAST, SPEED_EXTREME
};

static void redraw_speed_opts(GameModel *m, int cx, int cy) {
    for (int i = 0; i < 3; i++) {
        gotoxy(cx, cy + i * 2);
        if (i == m->speed_cursor)
            printf(ANSI_BYELLOW ANSI_BOLD SELECTOR
                   " \033[43m\033[30m\033[1m%-30s\033[0m" ANSI_RESET, SPEED_LABELS[i]);
        else
            printf(ANSI_WHITE "  %-30s" ANSI_RESET, SPEED_LABELS[i]);
    }
    fflush(stdout);
}

void view_draw_speed_select(GameModel *m) {
    /* Apenas atualiza o cursor (sem limpar tela) */
    redraw_speed_opts(m, 14, 6);
}

void view_draw_speed_select_full(GameModel *m) {
    clear_screen();
    int cx = 14, cy = 6;

    gotoxy(cx - 2, cy - 4);
    printf(ANSI_BYELLOW ANSI_BOLD "+==================================+" ANSI_RESET);
    gotoxy(cx - 2, cy - 3);
    printf(ANSI_BYELLOW ANSI_BOLD "|    SELECIONE A VELOCIDADE        |" ANSI_RESET);
    gotoxy(cx - 2, cy - 2);
    printf(ANSI_BYELLOW ANSI_BOLD "+==================================+" ANSI_RESET);

    /* Demonstracao visual de velocidade */
    gotoxy(cx - 2, cy + 9);
    printf(ANSI_BYELLOW "|" ANSI_DIM " Setas cima/baixo + ENTER para confirmar " ANSI_BYELLOW "|" ANSI_RESET);
    gotoxy(cx - 2, cy + 10);
    printf(ANSI_BYELLOW "+==================================+" ANSI_RESET);

    redraw_speed_opts(m, cx, cy);
}

int speed_select_value(int cursor) {
    return (cursor >= 0 && cursor < 3) ? SPEED_VALUES[cursor] : SPEED_NORMAL;
}

/* ════════════════════════════════════════════════════════
   PAUSA (M07)
   ════════════════════════════════════════════════════════ */
void view_draw_pause(GameModel *m) {
    int px = PANEL_X, r = 3;
    gotoxy(px, r++); printf(ANSI_BYELLOW "+=====================+" ANSI_RESET);
    gotoxy(px, r++); printf(ANSI_BYELLOW "|       PAUSADO       |" ANSI_RESET);
    gotoxy(px, r++); printf(ANSI_BYELLOW "+=====================+" ANSI_RESET);
    gotoxy(px, r++); printf(ANSI_BYELLOW "|" ANSI_WHITE " Score  : %9d  " ANSI_BYELLOW "|" ANSI_RESET, m->score);
    gotoxy(px, r++); printf(ANSI_BYELLOW "|" ANSI_WHITE " Nivel  : %9d  " ANSI_BYELLOW "|" ANSI_RESET, m->level);
    gotoxy(px, r++); printf(ANSI_BYELLOW "|" ANSI_WHITE " Vidas  : %9d  " ANSI_BYELLOW "|" ANSI_RESET, m->pacman.lives);
    gotoxy(px, r++); printf(ANSI_BYELLOW "|" ANSI_WHITE " Pontos : %5d/%4d" ANSI_BYELLOW "|" ANSI_RESET,
                            m->stats.total_dots - m->dot_count, m->stats.total_dots);
    gotoxy(px, r++); printf(ANSI_BYELLOW "|" ANSI_WHITE " Fant.  : %9d  " ANSI_BYELLOW "|" ANSI_RESET, m->stats.ghosts_eaten);
    gotoxy(px, r++); printf(ANSI_BYELLOW "|" ANSI_WHITE " BST alt: %9d  " ANSI_BYELLOW "|" ANSI_RESET, m->stats.bst_height);
    gotoxy(px, r++); printf(ANSI_BYELLOW "+=====================+" ANSI_RESET);
    gotoxy(px, r++); printf(ANSI_BYELLOW "|" ANSI_WHITE " [P] Continuar       " ANSI_BYELLOW "|" ANSI_RESET);
    gotoxy(px, r++); printf(ANSI_BYELLOW "|" ANSI_WHITE " [F] Finalizar       " ANSI_BYELLOW "|" ANSI_RESET);
    gotoxy(px, r++); printf(ANSI_BYELLOW "|" ANSI_WHITE " [Q] Sair (programa) " ANSI_BYELLOW "|" ANSI_RESET);
    gotoxy(px, r++); printf(ANSI_BYELLOW "+=====================+" ANSI_RESET);
    fflush(stdout);
}

/* ════════════════════════════════════════════════════════
   AJUDA (M08)
   ════════════════════════════════════════════════════════ */
void view_draw_help(GameModel *m) {
    (void)m;
    view_transition();
    int cx = 2, r = 0;
    gotoxy(cx, r++);
    printf(ANSI_BYELLOW ANSI_BOLD "=========== AJUDA ==========================" ANSI_RESET);
    r++;
    gotoxy(cx, r++); printf(ANSI_BWHITE " CONTROLES:" ANSI_RESET);
    gotoxy(cx, r++); printf("   Setas / WASD    Mover Pac-Man");
    gotoxy(cx, r++); printf("   [P]             Pausar / Retomar");
    gotoxy(cx, r++); printf("   [D] maiusc.     Debug de distancias");
    gotoxy(cx, r++); printf("   [H]             Esta tela");
    gotoxy(cx, r++); printf("   [Q]             Sair");
    r++;
    gotoxy(cx, r++); printf(ANSI_CYAN " PERSONAGENS:" ANSI_RESET);
    gotoxy(cx, r++);
    printf("  \033[40m" VIBRANT_YELLOW "\033[1m" PAC_RIGHT " \033[0m Pac-Man (boca aberta — direita)");
    gotoxy(cx, r++);
    printf("  \033[40m" VIBRANT_YELLOW "\033[1m" PAC_CLOSED " \033[0m Pac-Man (boca fechada)");
    gotoxy(cx, r++);
    printf("  \033[40m" CHERRY "\033[0m Cereja (deixa os fantasmas vulneraveis)");
    gotoxy(cx, r++);
    printf("  \033[41m" GHOST_EMOJI "\033[0m Blinky  "
           "\033[105m" GHOST_EMOJI "\033[0m Pinky  "
           "\033[46m" GHOST_EMOJI "\033[0m Inky  "
           "\033[43m" GHOST_EMOJI "\033[0m Clyde");
    gotoxy(cx, r++);
    printf("  \033[44m" GHOST_EMOJI "\033[0m Vulneravel (azul, pisca quando acabando)");
    r++;
    gotoxy(cx, r++); printf(ANSI_CYAN " ESTRUTURAS DE DADOS:" ANSI_RESET);
    gotoxy(cx, r++); printf(ANSI_BYELLOW " BST" ANSI_RESET " - Ranking. O(log n). ASCII art no Ranking.");
    gotoxy(cx, r++); printf(ANSI_MAGENTA " AVL" ANSI_RESET " - Power-ups. 4 rotacoes (LL RR LR RL).");
    gotoxy(cx, r++); printf(ANSI_GREEN " GRAFO" ANSI_RESET " - Labirinto (matriz + lista adj.)");
    gotoxy(cx, r++); printf(ANSI_WHITE " 7 SORTS" ANSI_RESET " - Bubble Sel Ins Shell Merge Quick Heap");
    r++;
    gotoxy(cx, r); printf(ANSI_DIM " [ENTER] Voltar ao menu" ANSI_RESET);
    fflush(stdout);
}

/* ════════════════════════════════════════════════════════
   GAME OVER
   ════════════════════════════════════════════════════════ */
void view_draw_game_over(GameModel *m) {
    view_transition();
    int cx = 10, r = 2;

    gotoxy(cx, r++); printf(ANSI_BRED ANSI_BOLD "  ###################################" ANSI_RESET);
    gotoxy(cx, r++); printf(ANSI_BRED ANSI_BOLD "  ##                               ##" ANSI_RESET);
    gotoxy(cx, r++); printf(ANSI_BRED ANSI_BOLD "  ##   ____    _    __  __ _____   ##" ANSI_RESET);
    gotoxy(cx, r++); printf(ANSI_BRED ANSI_BOLD "  ##  / ___|  / \\  |  \\/  | ____|  ##" ANSI_RESET);
    gotoxy(cx, r++); printf(ANSI_BRED ANSI_BOLD "  ##  | |  _ / _ \\ | |\\/| |  _|   ##" ANSI_RESET);
    gotoxy(cx, r++); printf(ANSI_BRED ANSI_BOLD "  ##  | |_| / ___ \\| |  | | |___  ##" ANSI_RESET);
    gotoxy(cx, r++); printf(ANSI_BRED ANSI_BOLD "  ##  \\____/_/   \\_\\_|  |_|_____| ##" ANSI_RESET);
    gotoxy(cx, r++); printf(ANSI_BRED ANSI_BOLD "  ##                               ##" ANSI_RESET);
    gotoxy(cx, r++); printf(ANSI_BRED ANSI_BOLD "  ##      * *  O V E R  * *        ##" ANSI_RESET);
    gotoxy(cx, r++); printf(ANSI_BRED ANSI_BOLD "  ###################################" ANSI_RESET);

    r += 2;
    gotoxy(cx, r++);
    printf(ANSI_BYELLOW " Score: " ANSI_BWHITE ANSI_BOLD "%d" ANSI_RESET
           ANSI_WHITE "   Nivel: " ANSI_BYELLOW "%d" ANSI_RESET, m->score, m->level);
    r++;
    memset(m->player_name, 0, sizeof(m->player_name));

    if (m->autoplay) {
        /* Modo demo: nao bloqueia esperando teclado — nome automatico */
        strncpy(m->player_name, "CPU (Auto)", 15);
        gotoxy(cx, r++);
        printf(ANSI_WHITE " Jogador: " ANSI_BYELLOW "CPU (Auto)" ANSI_RESET);
        fflush(stdout);
    } else {
        gotoxy(cx, r++);
        printf(ANSI_WHITE " Seu nome (ate 15 chars): " ANSI_RESET);
        show_cursor();
        fflush(stdout);

        /* P02: captura nome */
        int i = 0;
        int ch;
        while (i < 15) {
#ifdef _WIN32
            ch = _getch();
#else
            ch = getchar();
#endif
            if (ch == '\r' || ch == '\n') break;
            if ((ch == 0 || ch == 0xE0) && i == 0) {
#ifdef _WIN32
                _getch();
#endif
                continue;
            }
            if (ch == 8 || ch == 127) {
                if (i > 0) { i--; m->player_name[i] = 0; printf("\b \b"); fflush(stdout); }
                continue;
            }
            if (ch >= 32 && ch < 127) {
                m->player_name[i++] = (char)ch;
                printf("%c", ch); fflush(stdout);
            }
        }
        if (i == 0) strncpy(m->player_name, "Jogador", 15);
        hide_cursor();
    }

    m->score_tree = bst_insert(m->score_tree, m->score, m->player_name);
    model_save_scores(m);
    model_update_stats(m);

    m->state      = STATE_POST_GAME;
    m->post_cursor = 0;
    view_draw_post_game(m);
}

/* ════════════════════════════════════════════════════════
   POS-JOGO — Replay / Menu / Sair
   ════════════════════════════════════════════════════════ */
static const char *POST_ITEMS[3] = {
    "  Jogar Novamente  ",
    "  Voltar ao Menu   ",
    "  Sair             "
};

void view_draw_post_game(GameModel *m) {
    int cx = 18, cy = 19;
    gotoxy(cx - 2, cy++); printf(ANSI_BYELLOW "+========================+" ANSI_RESET);
    gotoxy(cx - 2, cy++); printf(ANSI_BYELLOW "|   O que deseja fazer?  |" ANSI_RESET);
    gotoxy(cx - 2, cy++); printf(ANSI_BYELLOW "+========================+" ANSI_RESET);
    for (int i = 0; i < 3; i++) {
        gotoxy(cx - 2, cy + i);
        if (i == m->post_cursor)
            printf(ANSI_BYELLOW ANSI_BOLD SELECTOR
                   " \033[43m\033[30m\033[1m%-20s\033[0m" ANSI_RESET, POST_ITEMS[i]);
        else
            printf(ANSI_WHITE "  %-20s" ANSI_RESET, POST_ITEMS[i]);
    }
    cy += 3;
    gotoxy(cx - 2, cy++); printf(ANSI_BYELLOW "+========================+" ANSI_RESET);
    gotoxy(cx - 2, cy);   printf(ANSI_DIM "  Setas + ENTER" ANSI_RESET);
    fflush(stdout);
}

/* ════════════════════════════════════════════════════════
   VITORIA
   ════════════════════════════════════════════════════════ */
void view_draw_win(GameModel *m) {
    view_transition();
    int cx = 12, r = 5;
    gotoxy(cx, r++); printf(ANSI_BYELLOW ANSI_BOLD "+=====================================+" ANSI_RESET);
    gotoxy(cx, r++); printf(ANSI_BYELLOW ANSI_BOLD "|                                     |" ANSI_RESET);
    gotoxy(cx, r++); printf(ANSI_BYELLOW ANSI_BOLD "|    V  O  C  E     V  E  N  C  E  U  |" ANSI_RESET);
    gotoxy(cx, r++); printf(ANSI_BYELLOW ANSI_BOLD "|          ! ! ! ! ! ! ! !            |" ANSI_RESET);
    gotoxy(cx, r++); printf(ANSI_BYELLOW ANSI_BOLD "|                                     |" ANSI_RESET);
    gotoxy(cx, r++); printf(ANSI_BYELLOW ANSI_BOLD "+=====================================+" ANSI_RESET);
    r += 2;
    gotoxy(cx + 2, r++);
    printf(ANSI_BWHITE " Score: " ANSI_BYELLOW ANSI_BOLD "%d" ANSI_RESET
           ANSI_BWHITE "   Nivel: " ANSI_BYELLOW ANSI_BOLD "%d" ANSI_RESET,
           m->score, m->level - 1);
    r++;
    gotoxy(cx + 2, r);
    printf(ANSI_DIM " [ENTER] Proxima fase" ANSI_RESET);
    fflush(stdout);
}

/* ════════════════════════════════════════════════════════
   RANKING (RF06 + M01 + M02)
   ════════════════════════════════════════════════════════ */
void view_draw_ranking(GameModel *m) {
    view_transition();
    int arr[MAX_SCORES]; char names[MAX_SCORES][16]; int cnt = 0;
    bst_inorder_arr(m->score_tree, arr, names, &cnt);
    /* Inverte para decrescente */
    for (int i = 0, j = cnt-1; i < j; i++, j--) {
        int t = arr[i]; arr[i] = arr[j]; arr[j] = t;
        char tmp[16]; memcpy(tmp, names[i], 16);
        memcpy(names[i], names[j], 16); memcpy(names[j], tmp, 16);
    }
    if (cnt > 0) run_all_sorts(m, arr, cnt);  /* M01 */

    int cx = 2, r = 0;
    gotoxy(cx, r++);
    printf(ANSI_BYELLOW ANSI_BOLD
           "======== RANKING DE HIGH SCORES ========" ANSI_RESET);
    r++;
    gotoxy(cx, r++); printf(ANSI_BWHITE " Pos  Nome            Score" ANSI_RESET);
    gotoxy(cx, r++); printf(ANSI_DIM " ---  --------------  ---------" ANSI_RESET);
    for (int i = 0; i < cnt && i < 10; i++) {
        gotoxy(cx, r++);
        const char *medal = (i==0)?ANSI_BYELLOW:(i==1)?ANSI_BWHITE:(i==2)?ANSI_YELLOW:ANSI_RESET;
        printf("%s %3d  %-14s  %7d" ANSI_RESET, medal, i+1, names[i], arr[i]);
    }
    if (!cnt) { gotoxy(cx, r++); printf(ANSI_DIM " (nenhum score)" ANSI_RESET); }

    r++;
    gotoxy(cx, r++);
    printf(ANSI_CYAN "---- Comparacao 7 Sorts (M01) -----------" ANSI_RESET);
    gotoxy(cx, r++); printf(ANSI_DIM " %-13s  %10s" ANSI_RESET, "Algoritmo", "Tempo(us)");
    long best = 9999999; int bi = 0;
    for (int s = 0; s < 7; s++)
        if (m->stats.sort_times_us[s] < best)
            { best = m->stats.sort_times_us[s]; bi = s; }
    for (int s = 0; s < 7; s++) {
        gotoxy(cx, r++);
        printf("%s %-13s  %8ld us%s%s",
               (s==bi)?ANSI_BYELLOW:ANSI_WHITE,
               m->stats.sort_names[s], m->stats.sort_times_us[s],
               ANSI_RESET, (s==bi)?ANSI_GREEN "  << mais rapido" ANSI_RESET:"");
    }
    r++;
    gotoxy(cx, r++);
    printf(ANSI_MAGENTA "---- BST ASCII Art (M02) ----------------" ANSI_RESET);
    if (m->score_tree) { gotoxy(cx, r); bst_print_ascii(m->score_tree, 0); }
    else { gotoxy(cx, r); printf(ANSI_DIM " (BST vazia)" ANSI_RESET); }

    gotoxy(cx, MAP_H + 2);
    printf(ANSI_DIM " [ENTER] Voltar ao menu" ANSI_RESET);
    fflush(stdout);
}

void view_draw_bst_ascii(GameModel *m) {
    gotoxy(0, MAP_H + 2);
    printf(ANSI_CYAN "---- BST ASCII Art ----\n" ANSI_RESET);
    bst_print_ascii(m->score_tree, 0);
    fflush(stdout);
}

void view_draw_final_report(GameModel *m) {
    clear_screen();
    gotoxy(0, 0);
    printf(ANSI_BYELLOW "======== RELATORIO FINAL ========\n\n" ANSI_RESET);
    printf(ANSI_BWHITE "Score final      : " ANSI_BYELLOW "%d\n" ANSI_RESET, m->score);
    printf(ANSI_BWHITE "Nivel alcancado  : " ANSI_BYELLOW "%d\n" ANSI_RESET, m->level);
    printf(ANSI_BWHITE "Vidas restantes  : " ANSI_BYELLOW "%d\n" ANSI_RESET, m->pacman.lives);
    printf(ANSI_BWHITE "Pontos comidos   : " ANSI_BYELLOW "%d / %d\n" ANSI_RESET,
           m->stats.dots_eaten, m->stats.total_dots);
    printf(ANSI_BWHITE "Fantasmas comidos: " ANSI_BYELLOW "%d\n" ANSI_RESET, m->stats.ghosts_eaten);
    printf(ANSI_CYAN "\n---- BST ----\n" ANSI_RESET);
    printf("Altura: %d  Nos: %d  Min: %d  Max: %d\n",
           m->stats.bst_height, m->stats.bst_count,
           m->stats.bst_min_score, m->stats.bst_max_score);
    printf(ANSI_MAGENTA "\n---- AVL ----\n" ANSI_RESET);
    printf("Rot.simples: %d  duplas: %d  balanco: %+d\n",
           m->stats.avl_rot_simple, m->stats.avl_rot_double, m->stats.avl_balance);
    printf(ANSI_GREEN "\n---- SORTS ----\n" ANSI_RESET);
    long best2 = 9999999; int bi2 = 0;
    for (int s = 0; s < 7; s++)
        if (m->stats.sort_times_us[s] > 0 && m->stats.sort_times_us[s] < best2)
            { best2 = m->stats.sort_times_us[s]; bi2 = s; }
    printf("Sort mais rapido: %s (%ld us)\n", m->stats.sort_names[bi2], best2);
    printf(ANSI_DIM "\nObrigado por jogar! ADS 3.o Periodo\n" ANSI_RESET);
    fflush(stdout);
}
