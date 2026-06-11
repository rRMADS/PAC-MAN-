/*
 * test.c — Testes automatizados PASS/FAIL (P07)
 *
 * Valida:
 *   · BST: insercao, busca, remocao, altura, min/max
 *   · AVL: insercao com rotacoes, fator de balanco
 *   · 7 Sorts: ordenacao correta de array fixo
 *   · Grafo: BFS e Dijkstra em grafo simples
 *
 * Compile: gcc -std=c11 -Wall -o test.exe test.c model/model.c
 * Execute: test.exe
 */

#include "pacman.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

static int pass = 0, fail = 0;

#define ASSERT(cond, msg) do { \
    if (cond) { printf(ANSI_GREEN "[PASS]" ANSI_RESET " %s\n", msg); pass++; } \
    else       { printf(ANSI_BRED "[FAIL]" ANSI_RESET " %s\n", msg); fail++; } \
} while(0)

/* ─── Helpers ─────────────────────────────────── */
static int arr_sorted(int *a, int n) {
    for (int i = 1; i < n; i++) if (a[i] < a[i-1]) return 0;
    return 1;
}

/* ─── Testes BST ─────────────────────────────── */
static void test_bst(void) {
    printf("\n" ANSI_CYAN "── BST ──────────────────────────────────" ANSI_RESET "\n");

    BSTNode *root = NULL;
    root = bst_insert(root, 500, "Alice");
    root = bst_insert(root, 300, "Bob");
    root = bst_insert(root, 700, "Carol");
    root = bst_insert(root, 200, "Dan");
    root = bst_insert(root, 400, "Eve");
    root = bst_insert(root, 600, "Frank");
    root = bst_insert(root, 800, "Grace");

    ASSERT(bst_count(root)  == 7,   "BST: contagem = 7 apos 7 insercoes");
    ASSERT(bst_height(root) == 3,   "BST: altura = 3 (arvore balanceada)");
    ASSERT(bst_min(root)    == 200, "BST: min = 200");
    ASSERT(bst_max(root)    == 800, "BST: max = 800");

    ASSERT(bst_search(root, 400) != NULL, "BST: busca 400 encontrado");
    ASSERT(bst_search(root, 999) == NULL, "BST: busca 999 nao encontrado");

    /* Remocao: no folha */
    root = bst_remove(root, 800);
    ASSERT(bst_search(root, 800) == NULL, "BST: remocao no folha (800)");
    ASSERT(bst_count(root) == 6,          "BST: contagem = 6 apos remocao folha");

    /* Remocao: no com um filho */
    root = bst_remove(root, 200);
    ASSERT(bst_search(root, 200) == NULL, "BST: remocao no 1 filho (200)");
    ASSERT(bst_count(root) == 5,          "BST: contagem = 5 apos remocao 1 filho");

    /* Remocao: no com dois filhos */
    root = bst_remove(root, 300);
    ASSERT(bst_search(root, 300) == NULL, "BST: remocao no 2 filhos (300)");
    ASSERT(bst_count(root) == 4,          "BST: contagem = 4 apos remocao 2 filhos");
    ASSERT(bst_search(root, 400) != NULL, "BST: no 400 ainda presente apos remocao do pai");

    /* Ordem crescente via inorder */
    int iarr[MAX_SCORES];
    char inames[MAX_SCORES][16];
    int idx = 0;
    bst_inorder_arr(root, iarr, inames, &idx);
    ASSERT(arr_sorted(iarr, idx), "BST: inorder retorna elementos em ordem crescente");

    bst_free(root);
    ASSERT(1, "BST: free sem crash");
}

/* ─── Testes AVL ─────────────────────────────── */
static void test_avl(void) {
    printf("\n" ANSI_MAGENTA "── AVL ──────────────────────────────────" ANSI_RESET "\n");
    Stats s = {0};

    AVLNode *root = NULL;
    /* Insere em ordem crescente — forcaria degeneracao em BST */
    for (int i = 1; i <= 7; i++)
        root = avl_insert(root, i * 10, "PowerUp", 50, &s);

    int h = avl_height(root);
    ASSERT(h <= 3, "AVL: altura <= 3 para 7 nos (autobalanceada)");

    int bf = avl_balance_factor(root);
    ASSERT(bf >= -1 && bf <= 1, "AVL: fator de balanco da raiz em [-1,1]");

    ASSERT(s.avl_rot_simple > 0 || s.avl_rot_double > 0,
           "AVL: pelo menos 1 rotacao ocorreu durante insercoes");

    /* Insercao LR — forca rotacao dupla */
    AVLNode *r2 = NULL;
    Stats s2 = {0};
    r2 = avl_insert(r2, 30, "A", 10, &s2);
    r2 = avl_insert(r2, 10, "B", 10, &s2);
    r2 = avl_insert(r2, 20, "C", 10, &s2);  /* LR rotation */
    ASSERT(s2.avl_rot_double >= 1, "AVL: rotacao dupla LR detectada");
    ASSERT(avl_balance_factor(r2) >= -1 && avl_balance_factor(r2) <= 1,
           "AVL: balanceado apos rotacao LR");

    /* Remocao */
    root = avl_remove(root, 40, &s);
    ASSERT(avl_balance_factor(root) >= -1 && avl_balance_factor(root) <= 1,
           "AVL: balanceado apos remocao");

    avl_free(root);
    avl_free(r2);
    ASSERT(1, "AVL: free sem crash");
}

/* ─── Testes Sorts ────────────────────────────── */
static void test_sorts(void) {
    printf("\n" ANSI_WHITE "── 7 SORTS ──────────────────────────────" ANSI_RESET "\n");
    int src[] = {64, 34, 25, 12, 22, 11, 90};
    int n = 7;
    int tmp[7];

    #define TEST_SORT(fn, label) do { \
        memcpy(tmp, src, n*sizeof(int)); \
        fn(tmp, n); \
        ASSERT(arr_sorted(tmp, n), label); \
    } while(0)

    TEST_SORT(bubble_sort,    "Sort: Bubble Sort ordena corretamente");
    TEST_SORT(selection_sort, "Sort: Selection Sort ordena corretamente");
    TEST_SORT(insertion_sort, "Sort: Insertion Sort ordena corretamente");
    TEST_SORT(shell_sort,     "Sort: Shell Sort ordena corretamente");
    TEST_SORT(merge_sort,     "Sort: Merge Sort ordena corretamente");
    TEST_SORT(quick_sort,     "Sort: Quick Sort ordena corretamente");
    TEST_SORT(heap_sort_alg,  "Sort: Heap Sort ordena corretamente");

    /* Array ja ordenado */
    int asc[] = {1,2,3,4,5};
    memcpy(tmp, asc, 5*sizeof(int));
    bubble_sort(tmp, 5);
    ASSERT(arr_sorted(tmp, 5), "Sort: Bubble Sort em array ja ordenado");

    /* Array reverso */
    int desc[] = {5,4,3,2,1};
    memcpy(tmp, desc, 5*sizeof(int));
    quick_sort(tmp, 5);
    ASSERT(arr_sorted(tmp, 5), "Sort: Quick Sort em array reverso");
}

/* ─── Testes Grafo ────────────────────────────── */
static void test_graph(void) {
    printf("\n" ANSI_GREEN "── GRAFO ────────────────────────────────" ANSI_RESET "\n");
    /* Grafo simples linear: 0-1-2-3-4 */
    int n = 5;
    Graph *g = graph_create(n);
    graph_add_edge(g, 0, 1, 1);
    graph_add_edge(g, 1, 2, 1);
    graph_add_edge(g, 2, 3, 1);
    graph_add_edge(g, 3, 4, 1);

    /* Verifica matriz de adjacencias (RF05) */
    ASSERT(g->adj_matrix[0*n+1] == 1, "Grafo: adj_matrix[0][1] = 1");
    ASSERT(g->adj_matrix[1*n+0] == 1, "Grafo: adj_matrix[1][0] = 1 (bidirecional)");
    ASSERT(g->adj_matrix[0*n+2] == 0, "Grafo: adj_matrix[0][2] = 0 (nao adjacente)");

    /* BFS */
    int dist[5], prev[5];
    bfs(g, 0, dist, prev);
    ASSERT(dist[0] == 0, "BFS: dist[0] = 0 (origem)");
    ASSERT(dist[1] == 1, "BFS: dist[1] = 1");
    ASSERT(dist[4] == 4, "BFS: dist[4] = 4 (caminho mais curto)");

    /* Dijkstra */
    int dd[5], dp[5];
    dijkstra(g, 0, dd, dp);
    ASSERT(dd[0] == 0, "Dijkstra: dist[0] = 0");
    ASSERT(dd[4] == 4, "Dijkstra: dist[4] = 4 (mesmo resultado que BFS para pesos 1)");
    ASSERT(dp[4] == 3, "Dijkstra: prev[4] = 3 (caminho correto)");

    /* DFS */
    int vis[5], ord[5], len = 0;
    dfs(g, 0, vis, ord, &len);
    ASSERT(len == 5,       "DFS: visita todos os 5 vertices");
    ASSERT(vis[4] == 1,    "DFS: vertice 4 marcado como visitado");
    ASSERT(ord[0] == 0,    "DFS: primeiro visitado e a origem (0)");

    graph_free(g);
    ASSERT(1, "Grafo: free sem crash");
}

/* ─── Resultado ───────────────────────────────── */
int main(void) {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

    printf(ANSI_BYELLOW ANSI_BOLD
           "═══════════════════════════════════════\n"
           "   SUITE DE TESTES — PAC-MAN em C      \n"
           "   ADS 3.o Periodo                     \n"
           "═══════════════════════════════════════\n"
           ANSI_RESET);

    test_bst();
    test_avl();
    test_sorts();
    test_graph();

    printf("\n" ANSI_BYELLOW "═══════════════════════════════════════\n" ANSI_RESET);
    printf(ANSI_GREEN "  PASS: %d" ANSI_RESET "   " ANSI_BRED "FAIL: %d" ANSI_RESET
           "   Total: %d\n", pass, fail, pass + fail);
    printf(ANSI_BYELLOW "═══════════════════════════════════════\n" ANSI_RESET);

    return fail > 0 ? 1 : 0;
}
