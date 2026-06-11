/*
 * pacman.h  —  Interface principal do projeto PAC-MAN em C
 * Estruturas de Dados Aplicadas ao Desenvolvimento de Jogos
 * ADS — 3.o Periodo
 *
 * Declara todos os tipos, structs, defines e prototipíos de funções.
 */

#ifndef PACMAN_H
#define PACMAN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ═══════════════════════════════════════════
   Dimensoes do mapa (classico 28×31)
   ═══════════════════════════════════════════ */
#define MAP_W        28
#define MAP_H        31
#define GHOSTS       4
#define MAX_SCORES   20
#define TUNNEL_ROW   14

/* ═══════════════════════════════════════════
   Tipos de celula
   ═══════════════════════════════════════════ */
#define CELL_WALL    0
#define CELL_DOT     1
#define CELL_POWER   2
#define CELL_EMPTY   3
#define CELL_DOOR    4   /* porta do covil dos fantasmas */
#define CELL_GHOST_H 5   /* interior do covil */
#define CELL_FRUIT   6   /* fruta bonus temporaria */
#define CELL_SPEED   7   /* turbo: Pac-Man se move mais rapido */
#define CELL_FREEZE  8   /* gelo: fantasmas congelam           */
#define CELL_SHIELD  9   /* escudo: absorve um hit             */
#define CELL_DOUBLE  10  /* 2x: pontos dobrados                */

/* ═══════════════════════════════════════════
   Direcoes
   ═══════════════════════════════════════════ */
#define DIR_NONE  0
#define DIR_UP    1
#define DIR_DOWN  2
#define DIR_LEFT  3
#define DIR_RIGHT 4

/* ═══════════════════════════════════════════
   Codigos ANSI de cor
   ═══════════════════════════════════════════ */
#define ANSI_RESET        "\033[0m"
#define ANSI_BOLD         "\033[1m"
#define ANSI_DIM          "\033[2m"
#define ANSI_BLACK        "\033[30m"
#define ANSI_RED          "\033[31m"
#define ANSI_GREEN        "\033[32m"
#define ANSI_BGREEN       "\033[92m"
#define ANSI_YELLOW       "\033[33m"
#define ANSI_BLUE         "\033[34m"
#define ANSI_MAGENTA      "\033[35m"
#define ANSI_CYAN         "\033[36m"
#define ANSI_WHITE        "\033[37m"
#define ANSI_BBLUE        "\033[94m"
#define ANSI_BYELLOW      "\033[93m"
#define ANSI_BRED         "\033[91m"
#define ANSI_BMAGENTA     "\033[95m"
#define ANSI_BCYAN        "\033[96m"
#define ANSI_BWHITE       "\033[97m"
#define ANSI_BG_BLUE      "\033[44m"
#define ANSI_BG_BLACK     "\033[40m"

/* ═══════════════════════════════════════════
   Configuracoes de jogo
   ═══════════════════════════════════════════ */
#define FRAME_MS        85      /* ~12 fps (padrao) */
#define SCORE_DOT       10
#define SCORE_POWER     50
#define SCORE_GHOST     200
#define POWER_DURATION  70      /* frames de modo assustado */
#define RESPAWN_DELAY   300     /* frames para um ponto/power reaparecer */
#define FRUIT_INTERVAL  200     /* frames entre aparicoes da fruta bonus */
#define FRUIT_DURATION  250     /* frames que a fruta fica visivel       */
#define SCORE_FRUIT     300     /* pontos por comer a fruta bonus        */
#define FRUIT_X         13      /* coluna da fruta no mapa               */
#define FRUIT_Y         17      /* linha da fruta no mapa                */

/* Scatter/Chase */
#define SCATTER_DURATION  120    /* frames em modo dispersao   */
#define CHASE_DURATION   1200    /* frames em modo perseguicao */

/* Vida extra */
#define EXTRA_LIFE_SCORE  10000

/* Popup de pontos */
#define POPUP_DURATION    25

/* Posicoes dos novos power-ups no mapa */
#define SPEED_X    9
#define SPEED_Y   17
#define FREEZE_X  18
#define FREEZE_Y  17
#define SHIELD_X   9
#define SHIELD_Y  11
#define DOUBLE_X  18
#define DOUBLE_Y  11

/* Intervalos de spawn (frames) */
#define SPEED_INTERVAL   200
#define FREEZE_INTERVAL  220
#define SHIELD_INTERVAL  240
#define DOUBLE_INTERVAL  210

/* Duracao do item no mapa antes de sumir (frames) */
#define SPEED_MAP_DUR    180
#define FREEZE_MAP_DUR   180
#define SHIELD_MAP_DUR   200
#define DOUBLE_MAP_DUR   180

/* Duracao do efeito (frames) */
#define SPEED_EFFECT_DUR   120
#define FREEZE_EFFECT_DUR   80
#define DOUBLE_EFFECT_DUR  150

/* Pontos por coletar item especial */
#define SCORE_POWERITEM  150

#define INF             999999
#define CELL_W          2       /* largura de cada celula no terminal (2 chars) */
#define PANEL_X         (MAP_W * CELL_W + 3)  /* coluna do painel lateral = 59 */

/* ═══════════════════════════════════════════
   Velocidades de jogo (ms por frame)
   ═══════════════════════════════════════════ */
#define SPEED_SLOW      280   /* ~3-4 cel/s — muito lenta */
#define SPEED_NORMAL    100   /* ~10 cel/s  — classico    */
#define SPEED_FAST       85   /* ~12 cel/s  — desafio     */
#define SPEED_EXTREME    50   /* ~20 cel/s  — impossivel  */

/* ═══════════════════════════════════════════
   Tipos de IA dos fantasmas
   ═══════════════════════════════════════════ */
#define AI_DIJKSTRA  0   /* Blinky — vermelho */
#define AI_BFS       1   /* Pinky  — rosa    */
#define AI_DFS       2   /* Inky   — ciano   */
#define AI_RANDOM    3   /* Clyde  — laranja */

/* ═══════════════════════════════════════════
   Grafo
   ═══════════════════════════════════════════ */
#define MAX_VERTICES (MAP_W * MAP_H)   /* 868 vertices */

/* ══════════════════════════════════════════════════════════════
   BST — Arvore Binaria de Busca para ranking de high scores
   O(log n) em insercao/busca/remocao (media)
   ══════════════════════════════════════════════════════════════ */
typedef struct BSTNode {
    int            score;
    char           name[16];
    struct BSTNode *left;
    struct BSTNode *right;
} BSTNode;

/* ══════════════════════════════════════════════════════════════
   AVL — Arvore AVL para inventario de power-ups
   Garante altura O(log n) com balanceamento automatico
   ══════════════════════════════════════════════════════════════ */
typedef struct AVLNode {
    int            power_id;
    char           name[16];
    int            duration;
    int            height;
    struct AVLNode *left;
    struct AVLNode *right;
} AVLNode;

/* ══════════════════════════════════════════════════════════════
   Grafo — Lista de adjacencias (para DFS, BFS, Dijkstra)
   ══════════════════════════════════════════════════════════════ */
typedef struct AdjNode {
    int             vertex;
    int             weight;
    struct AdjNode *next;
} AdjNode;

typedef struct {
    int      num_vertices;
    AdjNode **adj_list;     /* adj_list[v] → lista de vizinhos de v */
    char    *adj_matrix;    /* adj_matrix[u*N+v] = 1 se existe aresta */
} Graph;

/* ════════════════════════════════
   Fila — usada pelo BFS
   ════════════════════════════════ */
typedef struct {
    int *data;
    int  head, tail, size, cap;
} Queue;

/* ════════════════════════════════
   Min-Heap — usada pelo Dijkstra
   ════════════════════════════════ */
typedef struct { int vertex, dist; } HeapNode;
typedef struct { HeapNode *data; int size, cap; } MinHeap;

/* ═══════════════════════════════════════════
   Entidades do jogo
   ═══════════════════════════════════════════ */
typedef struct {
    int x, y;
    int dx, dy;           /* direcao atual */
    int next_dx, next_dy; /* direcao enfileirada pelo jogador */
    int lives;
    int invincible;       /* timer do power-up */
} PacMan;

typedef enum {
    GMODE_SCATTER,
    GMODE_CHASE,
    GMODE_FRIGHTENED,
    GMODE_EATEN
} GhostMode;

typedef struct {
    int       x, y;
    int       dx, dy;
    int       home_x, home_y;  /* posicao inicial */
    GhostMode mode;
    int       frighten_timer;
    char      color[16];       /* codigo ANSI de cor */
    char      name[8];
    int       ai_type;         /* AI_DIJKSTRA / AI_BFS / AI_DFS / AI_RANDOM */
    int       active;          /* 1 = ja saiu do covil */
    int       release_timer;   /* frames ate sair do covil */
} Ghost;

/* ═══════════════════════════════════════════
   Estado do jogo
   ═══════════════════════════════════════════ */
typedef enum {
    STATE_MENU,
    STATE_SPEED_SELECT,   /* selecao de velocidade antes do jogo */
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER,
    STATE_POST_GAME,      /* opcoes apos fim de partida (replay/menu/sair) */
    STATE_WIN,
    STATE_RANKING,
    STATE_HELP
} GameState;

/* ═══════════════════════════════════════════
   Estatisticas — exibidas no painel lateral
   ═══════════════════════════════════════════ */
typedef struct {
    int  avl_rot_simple;
    int  avl_rot_double;
    int  bst_height;
    int  bst_count;
    int  bst_min_score;
    int  bst_max_score;
    int  avl_balance;
    int  graph_vertices;
    int  graph_edges;
    long sort_times_us[7];  /* tempo em microsegundos de cada sort */
    int  ghosts_eaten;
    int  dots_eaten;
    int  total_dots;
    char sort_names[7][16];
} Stats;

/* ═══════════════════════════════════════════
   GameModel — estado COMPLETO do jogo
   Toda informacao fica aqui; nunca usar
   variaveis globais (passa-se *GameModel).
   ═══════════════════════════════════════════ */
typedef struct {
    int       grid[MAP_H][MAP_W];
    PacMan    pacman;
    Ghost     ghosts[GHOSTS];
    int       score;
    int       level;
    int       running;
    int       debug_mode;          /* tecla D: sobrepos distancias */
    GameState state;
    BSTNode  *score_tree;
    AVLNode  *powerup_tree;
    Graph    *maze_graph;
    int       dijkstra_dist[MAX_VERTICES];
    int       dijkstra_prev[MAX_VERTICES];
    int       bfs_dist[MAX_VERTICES];
    int       bfs_prev[MAX_VERTICES];
    int       dfs_visited[MAX_VERTICES];
    int       dfs_order[MAX_VERTICES];
    int       dfs_order_len;
    Stats     stats;
    char      player_name[16];
    int       power_active;   /* frames restantes de fright mode */
    int       frame_count;
    int       dot_count;      /* pontos restantes no mapa */
    int       needs_redraw;
    int       respawn_grid[MAP_H][MAP_W]; /* countdown de reapare. de cada celula */
    int       original_grid[MAP_H][MAP_W]; /* tipo original de cada celula do mapa */
    int       fruit_timer;    /* >0: fruta ativa com N frames restantes */
    /* Poderes especiais ativos */
    int speed_timer;         /* frames de turbo restantes         */
    int freeze_timer;        /* frames de congelamento restantes  */
    int double_timer;        /* frames de pontos 2x restantes     */
    int shield_active;       /* 1 = escudo ativo                  */
    /* Contadores de spawn dos itens no mapa */
    int speed_spawn;         /* frames ate proximo spawn turbo    */
    int freeze_spawn;        /* frames ate proximo spawn gelo     */
    int shield_spawn;        /* frames ate proximo spawn escudo   */
    int double_spawn;        /* frames ate proximo spawn 2x       */
    /* Timers de presenca dos itens no mapa */
    int speed_item_timer;
    int freeze_item_timer;
    int shield_item_timer;
    int double_item_timer;
    /* Scatter/Chase global */
    int phase_scatter;       /* 1=dispersao, 0=perseguicao        */
    int phase_timer;         /* frames ate trocar de fase         */
    /* Combo de fantasmas (200/400/800/1600) */
    int ghost_combo;
    /* Vida extra */
    int next_extra_life;
    /* Popup de pontos */
    int popup_timer;
    int popup_x, popup_y;
    int popup_val;
    /* Total de pontos comidos sem regredir */
    int dots_ever_eaten;
    /* Navegacao de menus */
    int       menu_cursor;    /* 0-4: posicao do seletor no menu principal */
    int       speed_cursor;   /* 0-3: posicao do seletor na tela de velocidade */
    int       post_cursor;    /* 0-2: posicao no menu pos-jogo */
    int       game_speed_ms;  /* delay por frame em ms (controlado pela velocidade) */
    int       autoplay;       /* 1 = IA controla o Pac-Man sozinho (modo demo) */
} GameModel;

/* ════════════════════════════════════════════════════════
   Prototípos — MODEL
   ════════════════════════════════════════════════════════ */

/* BST — O(log n) insercao/busca, O(n) traversal */
BSTNode *bst_create_node(int score, const char *name);
BSTNode *bst_insert(BSTNode *root, int score, const char *name);
BSTNode *bst_search(BSTNode *root, int score);
BSTNode *bst_remove(BSTNode *root, int score);
int      bst_height(BSTNode *root);          /* O(n) */
int      bst_count(BSTNode *root);           /* O(n) */
int      bst_min(BSTNode *root);             /* O(h) */
int      bst_max(BSTNode *root);             /* O(h) */
void     bst_inorder_arr(BSTNode *root, int *arr, char names[][16], int *idx);
void     bst_free(BSTNode *root);
void     bst_print_ascii(BSTNode *root, int space);

/* AVL — O(log n) com balanceamento automatico */
AVLNode *avl_create_node(int id, const char *name, int dur);
int      avl_height(AVLNode *n);
int      avl_balance_factor(AVLNode *n);     /* RF04 */
void     avl_update_height(AVLNode *n);
AVLNode *avl_rotate_right(AVLNode *y, Stats *s);
AVLNode *avl_rotate_left(AVLNode *x, Stats *s);
AVLNode *avl_rotate_left_right(AVLNode *n, Stats *s);
AVLNode *avl_rotate_right_left(AVLNode *n, Stats *s);
AVLNode *avl_insert(AVLNode *root, int id, const char *name, int dur, Stats *s);
AVLNode *avl_remove(AVLNode *root, int id, Stats *s);
void     avl_free(AVLNode *root);

/* Grafo — dupla representacao (RF05) */
Graph   *graph_create(int n);
void     graph_add_edge(Graph *g, int u, int v, int w);
void     graph_build_from_map(Graph *g, int grid[MAP_H][MAP_W]);
void     graph_free(Graph *g);
int      coord_to_vertex(int x, int y);      /* O(1) */
void     vertex_to_coord(int v, int *x, int *y); /* O(1) */

/* BFS — O(V+E), fantasma Pinky */
void     bfs(Graph *g, int src, int *dist, int *prev);

/* DFS — O(V+E), fantasma Inky */
void     dfs(Graph *g, int src, int *visited, int *order, int *len);

/* Dijkstra — O((V+E)log V), fantasma Blinky */
void     dijkstra(Graph *g, int src, int *dist, int *prev);

/* Fila e Min-Heap (auxiliares) */
Queue   *queue_create(int cap);
void     queue_push(Queue *q, int val);
int      queue_pop(Queue *q);
int      queue_empty(Queue *q);
void     queue_free(Queue *q);
MinHeap *minheap_create(int cap);
void     minheap_push(MinHeap *h, int v, int d);
HeapNode minheap_pop(MinHeap *h);
int      minheap_empty(MinHeap *h);
void     minheap_free(MinHeap *h);

/* 7 Algoritmos de Ordenacao (RF06) */
void bubble_sort(int *arr, int n);        /* O(n^2)    */
void selection_sort(int *arr, int n);     /* O(n^2)    */
void insertion_sort(int *arr, int n);     /* O(n^2)    */
void shell_sort(int *arr, int n);         /* O(n log n)*/
void merge_sort(int *arr, int n);         /* O(n log n)*/
void quick_sort(int *arr, int n);         /* O(n log n)*/
void heap_sort_alg(int *arr, int n);      /* O(n log n)*/
void run_all_sorts(GameModel *m, int *src, int n); /* M01 */

/* Logica do jogo */
void model_init(GameModel *m);
void model_reset_level(GameModel *m);
void model_parse_map(GameModel *m);
void model_update(GameModel *m);
void model_move_pacman(GameModel *m);
void model_move_ghosts(GameModel *m);
int  model_pacman_ai(GameModel *m);   /* auto-play: decide direcao do Pac-Man */
int  model_is_navigable(GameModel *m, int x, int y, int for_ghost);
void model_check_collisions(GameModel *m);
void model_check_win(GameModel *m);
int  model_ghost_ai(GameModel *m, int gi);  /* retorna DIR_* */
void model_update_stats(GameModel *m);

/* Persistencia (P01, P02) */
void model_save_scores(GameModel *m);
void model_load_scores(GameModel *m);

/* ════════════════════════════════════════════════════════
   Prototípos — VIEW
   ════════════════════════════════════════════════════════ */
void view_init(void);
void view_cleanup(void);
void view_draw_menu(GameModel *m);
void view_draw_game(GameModel *m);
void view_draw_ranking(GameModel *m);
void view_draw_pause(GameModel *m);
void view_draw_help(GameModel *m);
void view_draw_game_over(GameModel *m);
void view_draw_win(GameModel *m);
void view_reset_dirty(void);                  /* reseta dirty-tracking ao entrar no jogo */
void view_draw_bst_ascii(GameModel *m);       /* M02 */
void view_draw_final_report(GameModel *m);    /* P06 */
void view_draw_speed_select(GameModel *m);      /* selecao de velocidade (redraw cursor) */
void view_draw_speed_select_full(GameModel *m); /* selecao de velocidade (tela cheia) */
void view_draw_post_game(GameModel *m);         /* opcoes pos-partida */
void view_draw_menu_cursor(GameModel *m);       /* redraw apenas do cursor do menu */
int  speed_select_value(int cursor);            /* retorna ms da velocidade selecionada */
void view_transition(void);                   /* M04 */
void gotoxy(int x, int y);

/* ════════════════════════════════════════════════════════
   Prototípos — CONTROLLER
   ════════════════════════════════════════════════════════ */
void controller_run(GameModel *m);
void controller_handle_input(GameModel *m);
void controller_update(GameModel *m);

#endif /* PACMAN_H */
