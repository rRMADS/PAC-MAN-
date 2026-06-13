/*
 * controller/controller.c — Camada CONTROLLER do PAC-MAN
 *
 * Responsabilidades:
 *   · Navegacao por menus com setas (cima/baixo) + ENTER
 *   · Selecao de velocidade antes do jogo
 *   · Controle do Pac-Man (setas / WASD)
 *   · Opcoes pos-partida: Jogar Novamente / Menu / Sair
 */

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>   /* timeBeginPeriod/timeEndPeriod (link: -lwinmm) */
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#endif

#include "../pacman.h"

/* ══════════════════════════════════════════════════════
   SLEEP PORTAVEL
   ══════════════════════════════════════════════════════ */
static void sleep_ms(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

/* ══════════════════════════════════════════════════════
   KBHIT / GETCH PORTAVEL
   ══════════════════════════════════════════════════════ */
static int kb_hit(void) {
#ifdef _WIN32
    return _kbhit();
#else
    struct termios oldt, newt;
    int ch, oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if (ch != EOF) { ungetc(ch, stdin); return 1; }
    return 0;
#endif
}

static int read_key(void) {
#ifdef _WIN32
    int c = _getch();
    /* Tecla estendida (setas, F-keys): primeiro byte eh 0 ou 0xE0 */
    if (c == 0 || c == 0xE0) c = _getch() | 0x100;
    return c;
#else
    return getchar();
#endif
}

/* ══════════════════════════════════════════════════════
   HELPERS: INICIALIZAR NOVO JOGO
   ══════════════════════════════════════════════════════ */
static void start_new_game(GameModel *m) {
    int speed    = m->game_speed_ms > 0 ? m->game_speed_ms : SPEED_NORMAL;
    int autoplay = m->autoplay;
    BSTNode *tree = m->score_tree;

    if (m->maze_graph)  { graph_free(m->maze_graph);  m->maze_graph  = NULL; }
    if (m->powerup_tree){ avl_free(m->powerup_tree);  m->powerup_tree = NULL; }

    memset(m, 0, sizeof(GameModel));

    m->score_tree   = tree;
    m->running      = 1;
    m->game_speed_ms = speed;
    m->autoplay     = autoplay;
    m->level        = 1;

    model_parse_map(m);

    m->pacman.x = 12; m->pacman.y = 23;
    m->pacman.dx = 0; m->pacman.dy = 0;
    m->pacman.lives = 3;

    const char *gnames[4]  = {"Blinky","Pinky","Inky","Clyde"};
    const char *gcolors[4] = {ANSI_BRED, ANSI_BMAGENTA, ANSI_BCYAN, ANSI_BYELLOW};
    int gai[4]  = {AI_DIJKSTRA, AI_BFS, AI_DFS, AI_RANDOM};
    int ghx[4]  = {13, 13, 11, 15};
    int ghy[4]  = {11, 14, 14, 14};
    int grel[4] = {0, 30, 60, 90};

    for (int i = 0; i < GHOSTS; i++) {
        strncpy(m->ghosts[i].name,  gnames[i],  7);
        strncpy(m->ghosts[i].color, gcolors[i], 15);
        m->ghosts[i].ai_type       = gai[i];
        m->ghosts[i].x  = m->ghosts[i].home_x = ghx[i];
        m->ghosts[i].y  = m->ghosts[i].home_y = ghy[i];
        m->ghosts[i].mode          = GMODE_SCATTER;
        m->ghosts[i].release_timer = grel[i];
        m->ghosts[i].active        = (i == 0) ? 1 : 0;
    }

    const char *sn[7] = {"Bubble","Selection","Insertion","Shell","Merge","Quick","Heap"};
    for (int i = 0; i < 7; i++) strncpy(m->stats.sort_names[i], sn[i], 15);
    m->stats.total_dots = m->dot_count;

    m->maze_graph = graph_create(MAX_VERTICES);
    graph_build_from_map(m->maze_graph, m->grid);

    int pv = coord_to_vertex(m->pacman.x, m->pacman.y);
    bfs     (m->maze_graph, pv, m->bfs_dist,      m->bfs_prev);
    dijkstra(m->maze_graph, pv, m->dijkstra_dist,  m->dijkstra_prev);
    dfs     (m->maze_graph, pv, m->dfs_visited,    m->dfs_order, &m->dfs_order_len);

    view_reset_dirty();
    m->state = STATE_PLAYING;
}

/* Avancar para proxima fase (vitoria) */
static void next_level(GameModel *m) {
    int score = m->score;
    int lives = m->pacman.lives;
    int level = m->level;
    int speed = m->game_speed_ms;
    int autoplay = m->autoplay;
    BSTNode *tree = m->score_tree;

    if (m->maze_graph)   { graph_free(m->maze_graph);   m->maze_graph   = NULL; }
    if (m->powerup_tree) { avl_free(m->powerup_tree);   m->powerup_tree = NULL; }

    memset(m, 0, sizeof(GameModel));

    m->score_tree    = tree;
    m->score         = score;
    m->pacman.lives  = lives;
    m->level         = level;
    m->game_speed_ms = speed;
    m->autoplay      = autoplay;
    m->running       = 1;

    model_parse_map(m);
    m->pacman.x = 12; m->pacman.y = 23;

    const char *gnames[4]  = {"Blinky","Pinky","Inky","Clyde"};
    const char *gcolors[4] = {ANSI_BRED, ANSI_BMAGENTA, ANSI_BCYAN, ANSI_BYELLOW};
    int gai[4]  = {AI_DIJKSTRA, AI_BFS, AI_DFS, AI_RANDOM};
    int ghx[4]  = {13, 13, 11, 15};
    int ghy[4]  = {11, 14, 14, 14};
    int grel[4] = {0, 30, 60, 90};

    for (int i = 0; i < GHOSTS; i++) {
        strncpy(m->ghosts[i].name,  gnames[i], 7);
        strncpy(m->ghosts[i].color, gcolors[i], 15);
        m->ghosts[i].ai_type       = gai[i];
        m->ghosts[i].x  = m->ghosts[i].home_x = ghx[i];
        m->ghosts[i].y  = m->ghosts[i].home_y = ghy[i];
        m->ghosts[i].mode          = GMODE_SCATTER;
        m->ghosts[i].release_timer = grel[i];
        m->ghosts[i].active        = (i == 0) ? 1 : 0;
    }

    const char *sn[7] = {"Bubble","Selection","Insertion","Shell","Merge","Quick","Heap"};
    for (int i = 0; i < 7; i++) strncpy(m->stats.sort_names[i], sn[i], 15);
    m->stats.total_dots = m->dot_count;

    m->maze_graph = graph_create(MAX_VERTICES);
    graph_build_from_map(m->maze_graph, m->grid);

    int pv = coord_to_vertex(m->pacman.x, m->pacman.y);
    bfs(m->maze_graph, pv, m->bfs_dist, m->bfs_prev);
    dijkstra(m->maze_graph, pv, m->dijkstra_dist, m->dijkstra_prev);
    dfs(m->maze_graph, pv, m->dfs_visited, m->dfs_order, &m->dfs_order_len);

    view_reset_dirty();
    m->state = STATE_PLAYING;
}

/* ══════════════════════════════════════════════════════
   HANDLE INPUT — trata todas as teclas de acordo com
   o estado atual do jogo
   ══════════════════════════════════════════════════════ */
void controller_handle_input(GameModel *m) {
    if (!kb_hit()) return;
    int c = read_key();

    /* ─── TECLAS ESTENDIDAS (setas, etc.) ─── */
#ifdef _WIN32
    if (c & 0x100) {
        int sc = c & 0xFF;
#else
    if (c == 27) {
        /* Sequencias ANSI de escape: ESC [ A/B/C/D */
        if (!kb_hit()) return;
        read_key(); /* descarta '[' */
        if (!kb_hit()) return;
        int sc2 = read_key();
        /* Mapeia A/B/C/D para scan codes equivalentes */
        int sc = (sc2=='A')?72:(sc2=='B')?80:(sc2=='C')?77:(sc2=='D')?75:0;
        if (1) {
#endif
        switch (m->state) {

        /* Navegacao no menu principal */
        case STATE_MENU:
            if (sc == 72) { /* seta cima */
                m->menu_cursor = (m->menu_cursor - 1 + 5) % 5;
                view_draw_menu_cursor(m);
            } else if (sc == 80) { /* seta baixo */
                m->menu_cursor = (m->menu_cursor + 1) % 5;
                view_draw_menu_cursor(m);
            }
            break;

        /* Navegacao na selecao de velocidade */
        case STATE_SPEED_SELECT:
            if (sc == 72) {
                m->speed_cursor = (m->speed_cursor - 1 + 3) % 3;
                view_draw_speed_select(m);
            } else if (sc == 80) {
                m->speed_cursor = (m->speed_cursor + 1) % 3;
                view_draw_speed_select(m);
            }
            break;

        /* Navegacao no menu pos-jogo */
        case STATE_POST_GAME:
            if (sc == 72) {
                m->post_cursor = (m->post_cursor - 1 + 3) % 3;
                view_draw_post_game(m);
            } else if (sc == 80) {
                m->post_cursor = (m->post_cursor + 1) % 3;
                view_draw_post_game(m);
            }
            break;

        /* Movimento do Pac-Man */
        case STATE_PLAYING:
            if (sc == 72) { m->pacman.next_dx =  0; m->pacman.next_dy = -1; } /* cima */
            if (sc == 80) { m->pacman.next_dx =  0; m->pacman.next_dy =  1; } /* baixo */
            if (sc == 75) { m->pacman.next_dx = -1; m->pacman.next_dy =  0; } /* esq */
            if (sc == 77) { m->pacman.next_dx =  1; m->pacman.next_dy =  0; } /* dir */
            break;

        default: break;
        }
        return;
    } /* fim teclas estendidas */

    /* ─── TECLAS NORMAIS ─── */
    switch (c) {

    /* WASD para mover (minusculas) */
    case 'w': m->pacman.next_dx =  0; m->pacman.next_dy = -1; break;
    case 's': m->pacman.next_dx =  0; m->pacman.next_dy =  1; break;
    case 'a': m->pacman.next_dx = -1; m->pacman.next_dy =  0; break;
    case 'd': m->pacman.next_dx =  1; m->pacman.next_dy =  0; break;

    /* D maiusculo = toggle debug */
    case 'D':
        if (m->state == STATE_PLAYING || m->state == STATE_PAUSED)
            m->debug_mode = !m->debug_mode;
        break;

    /* Pausa */
    case 'p': case 'P':
        if (m->state == STATE_PLAYING) {
            m->state = STATE_PAUSED;
            view_draw_pause(m);
        } else if (m->state == STATE_PAUSED) {
            m->state = STATE_PLAYING;
        }
        break;

    /* Ajuda */
    case 'h': case 'H':
        if (m->state == STATE_MENU || m->state == STATE_PAUSED) {
            view_draw_help(m);
            m->state = STATE_HELP;
        }
        break;

    /* Sair do modo demo (auto-play) e voltar ao menu */
    case 'm': case 'M':
        if (m->state == STATE_PLAYING && m->autoplay) {
            m->autoplay = 0;
            m->state = STATE_MENU;
            view_draw_menu(m);
        }
        break;

    /* Finalizar a partida: salva o score e abre o menu pos-jogo */
    case 'f': case 'F':
        if (m->state == STATE_PLAYING || m->state == STATE_PAUSED) {
            m->state = STATE_GAME_OVER;
            view_draw_game_over(m);   /* salva score + nome -> STATE_POST_GAME */
        }
        break;

    /* ENTER / ESPACO — confirmar selecao */
    case '\r': case '\n': case ' ':
        if (m->state == STATE_MENU) {
            /* Ativa a opcao do cursor do menu */
            switch (m->menu_cursor) {
            case 0: /* Novo Jogo (jogador) */
                m->autoplay = 0;
                m->state = STATE_SPEED_SELECT;
                view_draw_speed_select_full(m);
                break;
            case 1: /* Auto-Play (IA joga sozinha) */
                m->autoplay = 1;
                m->state = STATE_SPEED_SELECT;
                view_draw_speed_select_full(m);
                break;
            case 2: /* Ranking */
                m->state = STATE_RANKING;
                view_draw_ranking(m);
                break;
            case 3: /* Ajuda */
                view_draw_help(m);
                m->state = STATE_HELP;
                break;
            case 4: /* Sair */
                view_draw_final_report(m);
                sleep_ms(3000);
                m->running = 0;
                break;
            }
        } else if (m->state == STATE_SPEED_SELECT) {
            /* Confirma velocidade e inicia jogo */
            m->game_speed_ms = speed_select_value(m->speed_cursor);
            start_new_game(m);
        } else if (m->state == STATE_POST_GAME) {
            /* Opcoes pos-jogo */
            switch (m->post_cursor) {
            case 0: /* Jogar Novamente */
                start_new_game(m);
                break;
            case 1: /* Voltar ao Menu */
                m->state = STATE_MENU;
                view_draw_menu(m);
                break;
            case 2: /* Sair */
                view_draw_final_report(m);
                sleep_ms(2000);
                m->running = 0;
                break;
            }
        } else if (m->state == STATE_RANKING ||
                   m->state == STATE_HELP    ||
                   m->state == STATE_WIN) {
            m->state = STATE_MENU;
            view_draw_menu(m);
        }
        break;

    /* Atalhos numericos no menu */
    case '1':
        if (m->state == STATE_MENU) {
            m->autoplay = 0;
            m->state = STATE_SPEED_SELECT;
            view_draw_speed_select_full(m);
        }
        break;
    case '2':
        if (m->state == STATE_MENU) {
            m->state = STATE_RANKING;
            view_draw_ranking(m);
        }
        break;

    /* Sair global */
    case 'q': case 'Q':
        if (m->state == STATE_PLAYING || m->state == STATE_PAUSED) {
            view_draw_final_report(m);
            sleep_ms(2000);
        }
        m->running = 0;
        break;
    }
}

/* ══════════════════════════════════════════════════════
   UPDATE — logica de jogo + transicoes de estado
   ══════════════════════════════════════════════════════ */
void controller_update(GameModel *m) {
    model_update(m);

    if (m->state == STATE_GAME_OVER) {
        /* view_draw_game_over ja faz entrada de nome e troca para POST_GAME */
        view_draw_game_over(m);

    } else if (m->state == STATE_WIN) {
        view_draw_win(m);
        sleep_ms(2000);
        next_level(m);
    }
}

/* ══════════════════════════════════════════════════════
   LOOP PRINCIPAL
   ══════════════════════════════════════════════════════ */
void controller_run(GameModel *m) {
#ifdef _WIN32
    /* Forca a resolucao do timer do sistema para 1 ms. SEM isto, o Sleep()
       do Windows tem granularidade de ~15,6 ms por padrao e arredonda os
       delays para cima — fazendo TODAS as velocidades ficarem lentas e
       parecidas em maquinas "limpas" (foi o que ocorreu no PC da professora).
       Com 1 ms, Sleep(25/45/65) fica preciso em qualquer computador. */
    timeBeginPeriod(1);
#endif
    view_init();
    model_init(m);

    /* Velocidade inicial: normal */
    m->game_speed_ms = SPEED_NORMAL;

    view_draw_menu(m);

    while (m->running) {
        controller_handle_input(m);

        if (m->state == STATE_PLAYING) {
            controller_update(m);
            /* Re-valida: update pode ter mudado para GAME_OVER/WIN */
            if (m->state == STATE_PLAYING)
                view_draw_game(m);
        } else if (m->state == STATE_PAUSED) {
            view_draw_game(m);
        }

        sleep_ms(m->game_speed_ms > 0 ? m->game_speed_ms : FRAME_MS);
    }

    view_cleanup();
#ifdef _WIN32
    timeEndPeriod(1);   /* devolve a resolucao do timer ao sistema */
#endif
}
