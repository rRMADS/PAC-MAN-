/*
 * model/model.c — Camada MODEL do PAC-MAN
 *
 * Contem:
 *   · BST  (high scores)            Cap. 4
 *   · AVL  (power-ups)              Cap. 5
 *   · Grafo duplo (labirinto)       Cap. 6
 *   · 7 Algoritmos de Ordenacao     Cap. 7
 *   · Logica completa do jogo
 *   · Persistencia em arquivo .dat  P01/P02
 */

#include "../pacman.h"

/* ══════════════════════════════════════════════════════════
   MAPA CLASSICO 28×31
   ══════════════════════════════════════════════════════════ */
static const char MAP_TEMPLATE[MAP_H][MAP_W + 1] = {
    "############################",  /*  0 */
    "#.....o......##......o.....#",  /*  1 */
    "#.####.#####.##.#####.####.#",  /*  2 */
    "#o####.#####.##.#####.####o#",  /*  3 */
    "#.####.#####.##.#####.####.#",  /*  4 */
    "#..........................#",  /*  5 */
    "#.####.##.########.##.####.#",  /*  6 */
    "#.####.##.########.##.####.#",  /*  7 */
    "#......##.o..##..o.##......#",  /*  8 */
    "######.#####.##.#####.######",  /*  9 */
    "     #.#####.##.#####.#     ",  /* 10 */
    "     #.##          ##.#     ",  /* 11 */
    "     #.## ###--### ##.#     ",  /* 12 */
    "######.## #      # ##.######",  /* 13 */
    "       .  #  GG  #  .       ",  /* 14 (tunel) */
    "######.## #      # ##.######",  /* 15 */
    "     #.## ######## ##.#     ",  /* 16 */
    "     #.##          ##.#     ",  /* 17 */
    "     #.## ######## ##.#     ",  /* 18 */
    "######.##.########.##.######",  /* 19 */
    "#............##............#",  /* 20 */
    "#.####.#####.##.#####.####.#",  /* 21 */
    "#o####.#####.##.#####.####o#",  /* 22 */
    "#.####.#####.##.#####.####.#",  /* 23 */
    "#......##....##....##......#",  /* 24 */
    "######.#####.##.#####.######",  /* 25 */
    "#.....o......##......o.....#",  /* 26 */
    "#.####.#####.##.#####.####.#",  /* 27 */
    "#o..##...............##...o#",  /* 28 */
    "#..........................#",  /* 29 */
    "############################"   /* 30 */
};

/* ══════════════════════════════════════════════════════════
   UTILIDADES DE COORDENADA / VERTICE
   ══════════════════════════════════════════════════════════ */

/* Converte (x,y) do mapa em indice de vertice — O(1) */
int coord_to_vertex(int x, int y) { return y * MAP_W + x; }

/* Converte vertice em coordenada — O(1) */
void vertex_to_coord(int v, int *x, int *y) {
    *x = v % MAP_W;
    *y = v / MAP_W;
}

/* ══════════════════════════════════════════════════════════
   BST — Arvore Binaria de Busca
   ══════════════════════════════════════════════════════════ */

/* Cria no da BST — O(1) */
BSTNode *bst_create_node(int score, const char *name) {
    BSTNode *n = (BSTNode *)malloc(sizeof(BSTNode));
    if (!n) { fprintf(stderr, "ERRO: malloc BST\n"); exit(1); }   /* P04 */
    n->score = score;
    strncpy(n->name, name, 15); n->name[15] = '\0';
    n->left = n->right = NULL;
    return n;
}

/* Insere score na BST — O(log n) media */
BSTNode *bst_insert(BSTNode *root, int score, const char *name) {
    if (!root) return bst_create_node(score, name);
    if (score < root->score)
        root->left  = bst_insert(root->left,  score, name);
    else if (score > root->score)
        root->right = bst_insert(root->right, score, name);
    /* score igual: ignora duplicatas */
    return root;
}

/* Busca score na BST — O(log n) media */
BSTNode *bst_search(BSTNode *root, int score) {
    if (!root || root->score == score) return root;
    if (score < root->score) return bst_search(root->left,  score);
    return                          bst_search(root->right, score);
}

/* Retorna no com menor chave (mais a esquerda) — O(h) */
static BSTNode *bst_min_node(BSTNode *root) {
    while (root && root->left) root = root->left;
    return root;
}

/* Remove score da BST com tratamento dos 3 casos — O(log n) */
BSTNode *bst_remove(BSTNode *root, int score) {
    if (!root) return NULL;
    if (score < root->score) {
        root->left  = bst_remove(root->left,  score);
    } else if (score > root->score) {
        root->right = bst_remove(root->right, score);
    } else {
        /* Caso 1: no folha */
        if (!root->left && !root->right) {
            free(root); return NULL;
        }
        /* Caso 2: um filho */
        if (!root->left)  { BSTNode *t = root->right; free(root); return t; }
        if (!root->right) { BSTNode *t = root->left;  free(root); return t; }
        /* Caso 3: dois filhos — substitui pelo sucessor in-order */
        BSTNode *succ = bst_min_node(root->right);
        root->score = succ->score;
        strncpy(root->name, succ->name, 15);
        root->right = bst_remove(root->right, succ->score);
    }
    return root;
}

/* Altura da arvore — O(n) */
int bst_height(BSTNode *root) {
    if (!root) return 0;
    int lh = bst_height(root->left);
    int rh = bst_height(root->right);
    return 1 + (lh > rh ? lh : rh);
}

/* Conta total de nos — O(n) */
int bst_count(BSTNode *root) {
    if (!root) return 0;
    return 1 + bst_count(root->left) + bst_count(root->right);
}

/* Menor score — O(h) */
int bst_min(BSTNode *root) {
    if (!root) return 0;
    while (root->left) root = root->left;
    return root->score;
}

/* Maior score — O(h) */
int bst_max(BSTNode *root) {
    if (!root) return 0;
    while (root->right) root = root->right;
    return root->score;
}

/* Percurso em ordem crescente, gravando em array — O(n) */
void bst_inorder_arr(BSTNode *root, int *arr, char names[][16], int *idx) {
    if (!root) return;
    bst_inorder_arr(root->left,  arr, names, idx);
    if (*idx < MAX_SCORES) {
        arr[*idx] = root->score;
        strncpy(names[*idx], root->name, 15);
        (*idx)++;
    }
    bst_inorder_arr(root->right, arr, names, idx);
}

/* Libera toda a arvore — O(n) */
void bst_free(BSTNode *root) {
    if (!root) return;
    bst_free(root->left);
    bst_free(root->right);
    free(root);
}

/* M02: Exibe a BST em ASCII art no terminal */
void bst_print_ascii(BSTNode *root, int space) {
    if (!root) return;
    space += 5;
    bst_print_ascii(root->right, space);
    printf("\n");
    for (int i = 5; i < space; i++) printf(" ");
    printf(ANSI_BYELLOW "%d" ANSI_RESET "\n", root->score);
    bst_print_ascii(root->left, space);
}

/* ══════════════════════════════════════════════════════════
   AVL — Arvore AVL autobalanceada
   ══════════════════════════════════════════════════════════ */

/* Cria no da AVL — O(1) */
AVLNode *avl_create_node(int id, const char *name, int dur) {
    AVLNode *n = (AVLNode *)malloc(sizeof(AVLNode));
    if (!n) { fprintf(stderr, "ERRO: malloc AVL\n"); exit(1); }   /* P04 */
    n->power_id = id;
    strncpy(n->name, name, 15); n->name[15] = '\0';
    n->duration = dur;
    n->height = 1;
    n->left = n->right = NULL;
    return n;
}

/* Retorna altura do no — O(1) */
int avl_height(AVLNode *n) { return n ? n->height : 0; }

/* Fator de balanceamento: h(esq) - h(dir) — O(1) */
int avl_balance_factor(AVLNode *n) {
    if (!n) return 0;
    return avl_height(n->left) - avl_height(n->right);
}

/* Atualiza altura — O(1) */
void avl_update_height(AVLNode *n) {
    if (!n) return;
    int lh = avl_height(n->left);
    int rh = avl_height(n->right);
    n->height = 1 + (lh > rh ? lh : rh);
}

/* Rotacao simples a direita — O(1) */
AVLNode *avl_rotate_right(AVLNode *y, Stats *s) {
    AVLNode *x  = y->left;
    AVLNode *T2 = x->right;
    x->right = y;
    y->left  = T2;
    avl_update_height(y);
    avl_update_height(x);
    s->avl_rot_simple++;    /* M05 */
    return x;
}

/* Rotacao simples a esquerda — O(1) */
AVLNode *avl_rotate_left(AVLNode *x, Stats *s) {
    AVLNode *y  = x->right;
    AVLNode *T2 = y->left;
    y->left  = x;
    x->right = T2;
    avl_update_height(x);
    avl_update_height(y);
    s->avl_rot_simple++;    /* M05 */
    return y;
}

/* Rotacao dupla esquerda-direita (LR) — O(1) */
AVLNode *avl_rotate_left_right(AVLNode *node, Stats *s) {
    node->left = avl_rotate_left(node->left, s);
    s->avl_rot_double++;    /* M05 */
    return avl_rotate_right(node, s);
}

/* Rotacao dupla direita-esquerda (RL) — O(1) */
AVLNode *avl_rotate_right_left(AVLNode *node, Stats *s) {
    node->right = avl_rotate_right(node->right, s);
    s->avl_rot_double++;    /* M05 */
    return avl_rotate_left(node, s);
}

/* Insere na AVL com rebalanceamento automatico — O(log n) */
AVLNode *avl_insert(AVLNode *root, int id, const char *name, int dur, Stats *s) {
    if (!root) return avl_create_node(id, name, dur);
    if (id < root->power_id)
        root->left  = avl_insert(root->left,  id, name, dur, s);
    else if (id > root->power_id)
        root->right = avl_insert(root->right, id, name, dur, s);
    else return root;

    avl_update_height(root);
    int bf = avl_balance_factor(root);

    if (bf > 1) {
        if (id < root->left->power_id) return avl_rotate_right(root, s);      /* LL */
        else                           return avl_rotate_left_right(root, s);  /* LR */
    }
    if (bf < -1) {
        if (id > root->right->power_id) return avl_rotate_left(root, s);      /* RR */
        else                            return avl_rotate_right_left(root, s); /* RL */
    }
    return root;
}

/* Retorna no com menor chave na AVL */
static AVLNode *avl_min_node(AVLNode *n) {
    while (n && n->left) n = n->left;
    return n;
}

/* Remove da AVL com rebalanceamento — O(log n) */
AVLNode *avl_remove(AVLNode *root, int id, Stats *s) {
    if (!root) return NULL;
    if (id < root->power_id) {
        root->left  = avl_remove(root->left,  id, s);
    } else if (id > root->power_id) {
        root->right = avl_remove(root->right, id, s);
    } else {
        if (!root->left || !root->right) {
            AVLNode *t = root->left ? root->left : root->right;
            free(root); return t;
        }
        AVLNode *succ = avl_min_node(root->right);
        root->power_id = succ->power_id;
        strncpy(root->name, succ->name, 15);
        root->duration = succ->duration;
        root->right = avl_remove(root->right, succ->power_id, s);
    }
    avl_update_height(root);
    int bf = avl_balance_factor(root);
    if (bf > 1) {
        if (avl_balance_factor(root->left) >= 0) return avl_rotate_right(root, s);
        else return avl_rotate_left_right(root, s);
    }
    if (bf < -1) {
        if (avl_balance_factor(root->right) <= 0) return avl_rotate_left(root, s);
        else return avl_rotate_right_left(root, s);
    }
    return root;
}

void avl_free(AVLNode *root) {
    if (!root) return;
    avl_free(root->left);
    avl_free(root->right);
    free(root);
}

/* ══════════════════════════════════════════════════════════
   FILA (BFS)
   ══════════════════════════════════════════════════════════ */
Queue *queue_create(int cap) {
    Queue *q = (Queue *)malloc(sizeof(Queue));
    if (!q) { fprintf(stderr, "ERRO: malloc Queue\n"); exit(1); }
    q->data = (int *)malloc(cap * sizeof(int));
    if (!q->data) { fprintf(stderr, "ERRO: malloc Queue data\n"); exit(1); }
    q->head = q->tail = q->size = 0;
    q->cap = cap;
    return q;
}
void queue_push(Queue *q, int val) {
    if (q->size == q->cap) return;
    q->data[q->tail] = val;
    q->tail = (q->tail + 1) % q->cap;
    q->size++;
}
int queue_pop(Queue *q) {
    int v = q->data[q->head];
    q->head = (q->head + 1) % q->cap;
    q->size--;
    return v;
}
int queue_empty(Queue *q) { return q->size == 0; }
void queue_free(Queue *q) { free(q->data); free(q); }

/* ══════════════════════════════════════════════════════════
   MIN-HEAP (Dijkstra)
   ══════════════════════════════════════════════════════════ */
MinHeap *minheap_create(int cap) {
    MinHeap *h = (MinHeap *)malloc(sizeof(MinHeap));
    if (!h) { fprintf(stderr, "ERRO: malloc MinHeap\n"); exit(1); }
    h->data = (HeapNode *)malloc(cap * sizeof(HeapNode));
    if (!h->data) { fprintf(stderr, "ERRO: malloc MinHeap data\n"); exit(1); }
    h->size = 0; h->cap = cap;
    return h;
}
void minheap_push(MinHeap *h, int v, int d) {
    if (h->size == h->cap) return;
    int i = h->size++;
    h->data[i].vertex = v; h->data[i].dist = d;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (h->data[p].dist <= h->data[i].dist) break;
        HeapNode tmp = h->data[p]; h->data[p] = h->data[i]; h->data[i] = tmp;
        i = p;
    }
}
HeapNode minheap_pop(MinHeap *h) {
    HeapNode top = h->data[0];
    h->data[0] = h->data[--h->size];
    int i = 0;
    for (;;) {
        int l = 2*i+1, r = 2*i+2, s = i;
        if (l < h->size && h->data[l].dist < h->data[s].dist) s = l;
        if (r < h->size && h->data[r].dist < h->data[s].dist) s = r;
        if (s == i) break;
        HeapNode tmp = h->data[s]; h->data[s] = h->data[i]; h->data[i] = tmp;
        i = s;
    }
    return top;
}
int  minheap_empty(MinHeap *h) { return h->size == 0; }
void minheap_free(MinHeap *h)  { free(h->data); free(h); }

/* ══════════════════════════════════════════════════════════
   GRAFO — dupla representacao (RF05)
   ══════════════════════════════════════════════════════════ */

/* Cria grafo com n vertices — O(n) */
Graph *graph_create(int n) {
    Graph *g = (Graph *)malloc(sizeof(Graph));
    if (!g) { fprintf(stderr, "ERRO: malloc Graph\n"); exit(1); }
    g->num_vertices = n;

    /* Lista de adjacencias */
    g->adj_list = (AdjNode **)calloc(n, sizeof(AdjNode *));
    if (!g->adj_list) { fprintf(stderr, "ERRO: malloc adj_list\n"); exit(1); }

    /* Matriz de adjacencias (byte por entrada para economizar mem) */
    g->adj_matrix = (char *)calloc(n * n, sizeof(char));
    if (!g->adj_matrix) { fprintf(stderr, "ERRO: malloc adj_matrix\n"); exit(1); }

    return g;
}

/* Adiciona aresta bidirecional — O(1) */
void graph_add_edge(Graph *g, int u, int v, int w) {
    if (u < 0 || v < 0 || u >= g->num_vertices || v >= g->num_vertices) return;
    /* Lista */
    AdjNode *nu = (AdjNode *)malloc(sizeof(AdjNode));
    if (!nu) { fprintf(stderr, "ERRO: malloc AdjNode\n"); exit(1); }
    nu->vertex = v; nu->weight = w; nu->next = g->adj_list[u];
    g->adj_list[u] = nu;

    AdjNode *nv = (AdjNode *)malloc(sizeof(AdjNode));
    if (!nv) { fprintf(stderr, "ERRO: malloc AdjNode\n"); exit(1); }
    nv->vertex = u; nv->weight = w; nv->next = g->adj_list[v];
    g->adj_list[v] = nv;

    /* Matriz */
    g->adj_matrix[u * g->num_vertices + v] = 1;
    g->adj_matrix[v * g->num_vertices + u] = 1;
}

/* Constroi grafo a partir do mapa (RF05) — O(MAP_W * MAP_H) */
void graph_build_from_map(Graph *g, int grid[MAP_H][MAP_W]) {
    int dx[] = {0,0,-1,1};
    int dy[] = {-1,1,0,0};

    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            if (grid[y][x] == CELL_WALL) continue;
            int u = coord_to_vertex(x, y);
            for (int d = 0; d < 4; d++) {
                int nx = x + dx[d];
                int ny = y + dy[d];
                /* Tratamento do tunel (linha 14) */
                if (ny == TUNNEL_ROW) {
                    if (nx < 0) nx = MAP_W - 1;
                    else if (nx >= MAP_W) nx = 0;
                }
                if (nx < 0 || nx >= MAP_W || ny < 0 || ny >= MAP_H) continue;
                if (grid[ny][nx] == CELL_WALL) continue;
                int v = coord_to_vertex(nx, ny);
                /* Evita duplicar arestas */
                if (!g->adj_matrix[u * g->num_vertices + v]) {
                    graph_add_edge(g, u, v, 1);
                }
            }
        }
    }
}

void graph_free(Graph *g) {
    if (!g) return;
    for (int i = 0; i < g->num_vertices; i++) {
        AdjNode *n = g->adj_list[i];
        while (n) { AdjNode *t = n->next; free(n); n = t; }
    }
    free(g->adj_list);
    free(g->adj_matrix);
    free(g);
}

/* ══════════════════════════════════════════════════════════
   BFS — Fantasma Pinky — O(V+E)
   Garante caminho mais curto em grafos nao ponderados.
   ══════════════════════════════════════════════════════════ */
void bfs(Graph *g, int src, int *dist, int *prev) {
    int n = g->num_vertices;
    for (int i = 0; i < n; i++) { dist[i] = INF; prev[i] = -1; }
    dist[src] = 0;
    Queue *q = queue_create(n);
    queue_push(q, src);
    while (!queue_empty(q)) {
        int u = queue_pop(q);
        for (AdjNode *a = g->adj_list[u]; a; a = a->next) {
            if (dist[a->vertex] == INF) {
                dist[a->vertex] = dist[u] + 1;
                prev[a->vertex] = u;
                queue_push(q, a->vertex);
            }
        }
    }
    queue_free(q);
}

/* ══════════════════════════════════════════════════════════
   DFS — Fantasma Inky — O(V+E)
   Explora profundamente antes de retroceder.
   ══════════════════════════════════════════════════════════ */
static void dfs_helper(Graph *g, int u, int *visited, int *order, int *len) {
    visited[u] = 1;
    if (*len < g->num_vertices) order[(*len)++] = u;
    for (AdjNode *a = g->adj_list[u]; a; a = a->next) {
        if (!visited[a->vertex])
            dfs_helper(g, a->vertex, visited, order, len);
    }
}

void dfs(Graph *g, int src, int *visited, int *order, int *len) {
    for (int i = 0; i < g->num_vertices; i++) visited[i] = 0;
    *len = 0;
    dfs_helper(g, src, visited, order, len);
}

/* ══════════════════════════════════════════════════════════
   DIJKSTRA — Fantasma Blinky — O((V+E) log V)
   Caminho mais curto com pesos (peso 1 por celula normal,
   peso diferente para tuneis).
   ══════════════════════════════════════════════════════════ */
void dijkstra(Graph *g, int src, int *dist, int *prev) {
    int n = g->num_vertices;
    for (int i = 0; i < n; i++) { dist[i] = INF; prev[i] = -1; }
    dist[src] = 0;
    MinHeap *h = minheap_create(n * 2);
    minheap_push(h, src, 0);
    while (!minheap_empty(h)) {
        HeapNode cur = minheap_pop(h);
        int u = cur.vertex;
        if (cur.dist > dist[u]) continue;
        for (AdjNode *a = g->adj_list[u]; a; a = a->next) {
            int nd = dist[u] + a->weight;
            if (nd < dist[a->vertex]) {
                dist[a->vertex] = nd;
                prev[a->vertex] = u;
                minheap_push(h, a->vertex, nd);
            }
        }
    }
    minheap_free(h);
}

/* ══════════════════════════════════════════════════════════
   7 ALGORITMOS DE ORDENACAO (RF06)
   ══════════════════════════════════════════════════════════ */

/* Bubble Sort — O(n^2) piorcaso, O(n) melhor */
void bubble_sort(int *arr, int n) {
    for (int i = 0; i < n-1; i++) {
        int sw = 0;
        for (int j = 0; j < n-1-i; j++) {
            if (arr[j] > arr[j+1]) {
                int t = arr[j]; arr[j] = arr[j+1]; arr[j+1] = t; sw = 1;
            }
        }
        if (!sw) break;
    }
}

/* Selection Sort — O(n^2) todos os casos */
void selection_sort(int *arr, int n) {
    for (int i = 0; i < n-1; i++) {
        int m = i;
        for (int j = i+1; j < n; j++)
            if (arr[j] < arr[m]) m = j;
        if (m != i) { int t = arr[i]; arr[i] = arr[m]; arr[m] = t; }
    }
}

/* Insertion Sort — O(n^2) pior, O(n) melhor */
void insertion_sort(int *arr, int n) {
    for (int i = 1; i < n; i++) {
        int key = arr[i], j = i-1;
        while (j >= 0 && arr[j] > key) { arr[j+1] = arr[j]; j--; }
        arr[j+1] = key;
    }
}

/* Shell Sort — O(n log^2 n) medio */
void shell_sort(int *arr, int n) {
    for (int gap = n/2; gap > 0; gap /= 2) {
        for (int i = gap; i < n; i++) {
            int tmp = arr[i], j = i;
            while (j >= gap && arr[j-gap] > tmp) { arr[j] = arr[j-gap]; j -= gap; }
            arr[j] = tmp;
        }
    }
}

/* Merge Sort auxiliar */
static void merge_helper(int *arr, int l, int m, int r) {
    int n1 = m-l+1, n2 = r-m;
    int *L = (int*)malloc(n1*sizeof(int));
    int *R = (int*)malloc(n2*sizeof(int));
    if (!L || !R) { free(L); free(R); return; }
    for (int i = 0; i < n1; i++) L[i] = arr[l+i];
    for (int j = 0; j < n2; j++) R[j] = arr[m+1+j];
    int i=0, j=0, k=l;
    while (i<n1 && j<n2) arr[k++] = (L[i]<=R[j]) ? L[i++] : R[j++];
    while (i<n1) arr[k++] = L[i++];
    while (j<n2) arr[k++] = R[j++];
    free(L); free(R);
}
static void merge_rec(int *arr, int l, int r) {
    if (l < r) {
        int m = l + (r-l)/2;
        merge_rec(arr, l, m);
        merge_rec(arr, m+1, r);
        merge_helper(arr, l, m, r);
    }
}
/* Merge Sort — O(n log n) todos os casos */
void merge_sort(int *arr, int n) { if (n > 1) merge_rec(arr, 0, n-1); }

/* Quick Sort auxiliar */
static int qs_partition(int *arr, int l, int r) {
    int p = arr[r], i = l-1;
    for (int j = l; j < r; j++) {
        if (arr[j] <= p) { i++; int t=arr[i]; arr[i]=arr[j]; arr[j]=t; }
    }
    int t = arr[i+1]; arr[i+1] = arr[r]; arr[r] = t;
    return i+1;
}
static void qs_rec(int *arr, int l, int r) {
    if (l < r) {
        int p = qs_partition(arr, l, r);
        qs_rec(arr, l, p-1);
        qs_rec(arr, p+1, r);
    }
}
/* Quick Sort — O(n log n) medio */
void quick_sort(int *arr, int n) { if (n > 1) qs_rec(arr, 0, n-1); }

/* Heap Sort auxiliar */
static void hs_heapify(int *arr, int n, int i) {
    int lg = i, l = 2*i+1, r = 2*i+2;
    if (l < n && arr[l] > arr[lg]) lg = l;
    if (r < n && arr[r] > arr[lg]) lg = r;
    if (lg != i) { int t=arr[i]; arr[i]=arr[lg]; arr[lg]=t; hs_heapify(arr,n,lg); }
}
/* Heap Sort — O(n log n) todos os casos */
void heap_sort_alg(int *arr, int n) {
    for (int i = n/2-1; i >= 0; i--) hs_heapify(arr, n, i);
    for (int i = n-1; i > 0; i--) {
        int t = arr[0]; arr[0] = arr[i]; arr[i] = t;
        hs_heapify(arr, i, 0);
    }
}

/* M01: Executa todos os 7 sorts medindo tempo em microssegundos */
void run_all_sorts(GameModel *m, int *src, int n) {
    if (n <= 0) return;
    int *tmp = (int *)malloc(n * sizeof(int));
    if (!tmp) return;

    void (*sorts[7])(int*, int) = {
        bubble_sort, selection_sort, insertion_sort,
        shell_sort, merge_sort, quick_sort, heap_sort_alg
    };
    const char *names[7] = {
        "Bubble","Selection","Insertion","Shell","Merge","Quick","Heap"
    };

    for (int s = 0; s < 7; s++) {
        memcpy(tmp, src, n * sizeof(int));
        clock_t t0 = clock();
        sorts[s](tmp, n);
        clock_t t1 = clock();
        m->stats.sort_times_us[s] =
            (long)((double)(t1 - t0) / CLOCKS_PER_SEC * 1000000.0);
        strncpy(m->stats.sort_names[s], names[s], 15);
    }
    free(tmp);
}

/* ══════════════════════════════════════════════════════════
   PARSE DO MAPA — converte template em grid inteiro
   ══════════════════════════════════════════════════════════ */
void model_parse_map(GameModel *m) {
    m->dot_count = 0;
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            char c = MAP_TEMPLATE[y][x];
            int cell;
            switch (c) {
                case '#': cell = CELL_WALL;    break;
                case '.': cell = CELL_DOT;     m->dot_count++; break;
                case 'o': cell = CELL_POWER;   m->dot_count++; break;
                case '-': cell = CELL_DOOR;    break;
                case 'G': cell = CELL_GHOST_H; break;
                case ' ':
                    /* Fora dos limites laterais nas linhas do corredor */
                    if (y >= 10 && y <= 18 && (x < 5 || x > 22))
                        cell = CELL_WALL;
                    /* Interior do covil (linhas 13-15, colunas 11-16) */
                    else if (y >= 13 && y <= 15 && x >= 11 && x <= 16)
                        cell = CELL_GHOST_H;
                    else
                        cell = CELL_EMPTY;
                    break;
                default:  cell = CELL_EMPTY;   break;
            }
            m->grid[y][x]          = cell;
            m->original_grid[y][x] = cell;   /* tipo original p/ reaparecimento */
            m->respawn_grid[y][x]  = 0;      /* sem reaparecimento pendente     */
        }
    }
    m->stats.total_dots = m->dot_count;
}

/* ══════════════════════════════════════════════════════════
   INICIALIZACAO DO MODELO
   ══════════════════════════════════════════════════════════ */
void model_init(GameModel *m) {
    memset(m, 0, sizeof(GameModel));
    m->running    = 1;
    m->state      = STATE_MENU;
    m->level      = 1;
    m->debug_mode = 0;

    /* Inicializa estatisticas dos sorts */
    const char *sn[7] = {"Bubble","Selection","Insertion","Shell","Merge","Quick","Heap"};
    for (int i = 0; i < 7; i++) strncpy(m->stats.sort_names[i], sn[i], 15);

    model_parse_map(m);

    /* Pac-Man — posicao inicial */
    m->pacman.x      = 12;
    m->pacman.y      = 23;
    m->pacman.dx     = 0;
    m->pacman.dy     = 0;
    m->pacman.lives  = 3;

    /* Fantasmas */
    const char *gnames[4]  = {"Blinky","Pinky","Inky","Clyde"};
    const char *gcolors[4] = {ANSI_BRED, ANSI_BMAGENTA, ANSI_BCYAN, ANSI_BYELLOW};
    int gai[4]   = {AI_DIJKSTRA, AI_BFS, AI_DFS, AI_RANDOM};
    int ghx[4]   = {13, 13, 11, 15};
    int ghy[4]   = {11, 14, 14, 14};
    int grel[4]  = {0, 30, 60, 90};   /* frames ate sair do covil */

    for (int i = 0; i < GHOSTS; i++) {
        strncpy(m->ghosts[i].name,  gnames[i],  7);
        strncpy(m->ghosts[i].color, gcolors[i], 15);
        m->ghosts[i].ai_type       = gai[i];
        m->ghosts[i].x  = m->ghosts[i].home_x = ghx[i];
        m->ghosts[i].y  = m->ghosts[i].home_y = ghy[i];
        m->ghosts[i].mode          = GMODE_SCATTER;
        m->ghosts[i].release_timer = grel[i];
        m->ghosts[i].active        = (i == 0) ? 1 : 0;  /* Blinky ativo imediatamente */
    }

    /* Grafo */
    m->maze_graph = graph_create(MAX_VERTICES);
    graph_build_from_map(m->maze_graph, m->grid);

    /* Conta arestas para as estatisticas */
    int edges = 0;
    for (int v = 0; v < MAX_VERTICES; v++) {
        for (AdjNode *a = m->maze_graph->adj_list[v]; a; a = a->next)
            if (a->vertex > v) edges++;
    }
    m->stats.graph_vertices = MAX_VERTICES;
    m->stats.graph_edges    = edges;

    /* Carrega scores salvos */
    model_load_scores(m);

    /* Pre-calcula BFS e Dijkstra a partir do Pac-Man */
    int pac_v = coord_to_vertex(m->pacman.x, m->pacman.y);
    bfs     (m->maze_graph, pac_v, m->bfs_dist,      m->bfs_prev);
    dijkstra(m->maze_graph, pac_v, m->dijkstra_dist,  m->dijkstra_prev);
    dfs     (m->maze_graph, pac_v, m->dfs_visited, m->dfs_order, &m->dfs_order_len);
}

/* ══════════════════════════════════════════════════════════
   VERIFICACAO DE NAVEGABILIDADE
   ══════════════════════════════════════════════════════════ */
int model_is_navigable(GameModel *m, int x, int y, int for_ghost) {
    /* Tunel: wrap horizontal na linha 14 */
    if (y == TUNNEL_ROW) {
        if (x < 0) x = MAP_W - 1;
        else if (x >= MAP_W) x = 0;
    }
    if (x < 0 || x >= MAP_W || y < 0 || y >= MAP_H) return 0;
    int c = m->grid[y][x];
    if (c == CELL_WALL) return 0;
    if (!for_ghost && (c == CELL_DOOR || c == CELL_GHOST_H)) return 0;
    return 1;
}

/* ══════════════════════════════════════════════════════════
   IA DOS FANTASMAS
   ══════════════════════════════════════════════════════════ */

/* Converte um passo (dx,dy) em DIR_* */
static int step_to_dir(int dx, int dy) {
    if (dx == 1) return DIR_RIGHT;
    if (dx ==-1) return DIR_LEFT;
    if (dy == 1) return DIR_DOWN;
    if (dy ==-1) return DIR_UP;
    return DIR_NONE;
}

/* NUCLEO ANTI-OSCILACAO — escolhe o melhor vizinho navegavel SEM dar
   meia-volta, minimizando o campo de distancia 'dist[]' (BFS/Dijkstra a
   partir do Pac-Man). Como nunca reverte (exceto em beco sem saida) e
   sempre desce a distancia real do grafo (navega ao redor de obstaculos
   como o covil), o fantasma progride ate o alvo e JAMAIS fica oscilando. */
static int ghost_step_by_dist(GameModel *m, int gi, const int *dist) {
    Ghost *g = &m->ghosts[gi];
    int dirs[4][2] = {{0,-1},{0,1},{-1,0},{1,0}};
    int best = DIR_NONE, best_d = INF, reverse_dir = DIR_NONE;
    for (int d = 0; d < 4; d++) {
        int nx = g->x + dirs[d][0];
        int ny = g->y + dirs[d][1];
        if (ny == TUNNEL_ROW) { if (nx < 0) nx = MAP_W-1; else if (nx >= MAP_W) nx = 0; }
        if (!model_is_navigable(m, nx, ny, 1)) continue;
        int dir = step_to_dir(dirs[d][0], dirs[d][1]);
        if (dirs[d][0] == -g->dx && dirs[d][1] == -g->dy) { reverse_dir = dir; continue; }
        int nd = dist[coord_to_vertex(nx, ny)];
        if (nd < best_d) { best_d = nd; best = dir; }
    }
    if (best != DIR_NONE) return best;   /* avanca sem reverter */
    return reverse_dir;                  /* beco sem saida: permite a meia-volta */
}

/* Blinky (Dijkstra) — caminho otimo ate o Pac-Man, sem oscilar. O((V+E)log V) */
static int ghost_dijkstra(GameModel *m, int gi) {
    int pv = coord_to_vertex(m->pacman.x, m->pacman.y);
    dijkstra(m->maze_graph, pv, m->dijkstra_dist, m->dijkstra_prev);
    return ghost_step_by_dist(m, gi, m->dijkstra_dist);
}

/* Pinky (BFS) — caminho mais curto ate o Pac-Man, sem oscilar. O(V+E) */
static int ghost_bfs(GameModel *m, int gi) {
    int pv = coord_to_vertex(m->pacman.x, m->pacman.y);
    bfs(m->maze_graph, pv, m->bfs_dist, m->bfs_prev);
    return ghost_step_by_dist(m, gi, m->bfs_dist);
}

/* Retorna a proxima direcao usando DFS INFORMADO — O(V+E)
   Busca em profundidade ate a celula do Pac-Man, mas em cada vertice
   explora primeiro o vizinho mais proximo do destino (heuristica de
   Manhattan). Continua sendo um DFS com backtracking, porem "mergulha"
   em direcao ao Pac-Man em vez de vagar pela ordem de adjacencia —
   antes o Inky avancava pouquissimo e parecia perdido. */
static int dfs_chase_helper(Graph *gr, int u, int dst, int *visited, int *parent) {
    visited[u] = 1;
    if (u == dst) return 1;

    /* Coleta vizinhos nao visitados e ordena por proximidade ao destino */
    int nb[4], cost[4], cnt = 0;
    int dx, dy, tx, ty;
    vertex_to_coord(dst, &tx, &ty);
    for (AdjNode *a = gr->adj_list[u]; a && cnt < 4; a = a->next) {
        if (visited[a->vertex]) continue;
        vertex_to_coord(a->vertex, &dx, &dy);
        nb[cnt]   = a->vertex;
        cost[cnt] = abs(dx - tx) + abs(dy - ty);
        cnt++;
    }
    /* Insertion sort (cnt <= 4): mais perto do Pac-Man primeiro */
    for (int i = 1; i < cnt; i++) {
        int kv = nb[i], kc = cost[i], j = i - 1;
        while (j >= 0 && cost[j] > kc) { nb[j+1] = nb[j]; cost[j+1] = cost[j]; j--; }
        nb[j+1] = kv; cost[j+1] = kc;
    }
    for (int i = 0; i < cnt; i++) {
        if (visited[nb[i]]) continue;
        parent[nb[i]] = u;
        if (dfs_chase_helper(gr, nb[i], dst, visited, parent)) return 1;
    }
    return 0;
}

static int ghost_dfs(GameModel *m, int gi) {
    Ghost *g   = &m->ghosts[gi];
    int    src = coord_to_vertex(g->x, g->y);
    int    dst = coord_to_vertex(m->pacman.x, m->pacman.y);
    if (src == dst) return DIR_NONE;

    Graph *gr = m->maze_graph;
    int   *parent  = m->dfs_order;     /* reaproveita buffers de tamanho V */
    int   *visited = m->dfs_visited;
    for (int v = 0; v < gr->num_vertices; v++) { parent[v] = -1; visited[v] = 0; }

    int dir = DIR_NONE;
    if (dfs_chase_helper(gr, src, dst, visited, parent)) {
        /* Reconstroi o caminho dst -> src e pega o 1o passo a partir de src */
        int cur = dst, prev = dst;
        while (cur != -1 && cur != src) { prev = cur; cur = parent[cur]; }
        if (cur == src) {
            int nx, ny;
            vertex_to_coord(prev, &nx, &ny);
            dir = step_to_dir(nx - g->x, ny - g->y);
        }
    }

    /* Guarda anti-oscilacao: se o passo do DFS daria meia-volta (ou falhou),
       desce o campo BFS sem reverter — continua "DFS", mas nunca trava. */
    int is_rev = (dir == DIR_UP    && g->dy ==  1) || (dir == DIR_DOWN  && g->dy == -1) ||
                 (dir == DIR_LEFT  && g->dx ==  1) || (dir == DIR_RIGHT && g->dx == -1);
    if (dir == DIR_NONE || is_rev) {
        bfs(gr, dst, m->bfs_dist, m->bfs_prev);
        dir = ghost_step_by_dist(m, gi, m->bfs_dist);
    }
    return dir;
}

/* Clyde (laranja) — comportamento classico do Pac-Man:
   persegue o Pac-Man de forma gulosa quando esta LONGE (>8 celulas) e
   anda de forma erratica quando chega perto, simulando timidez. Mesmo
   "aleatorio", ele agora pressiona o jogador em vez de vagar a esmo. */
static int ghost_random(GameModel *m, int gi) {
    Ghost *g = &m->ghosts[gi];
    int dirs[4][2] = {{0,-1},{0,1},{-1,0},{1,0}};

    /* 1 em cada 4 passos: direcao aleatoria valida SEM meia-volta —
       mantem a personalidade erratica sem ficar preso oscilando. */
    if (rand() % 4 == 0) {
        int valid[4], vc = 0;
        for (int d = 0; d < 4; d++) {
            int nx = g->x + dirs[d][0], ny = g->y + dirs[d][1];
            if (ny == TUNNEL_ROW) { if (nx < 0) nx = MAP_W-1; else if (nx >= MAP_W) nx = 0; }
            if (model_is_navigable(m, nx, ny, 1) &&
                !(dirs[d][0] == -g->dx && dirs[d][1] == -g->dy))
                valid[vc++] = d;
        }
        if (vc) { int c = valid[rand() % vc]; return step_to_dir(dirs[c][0], dirs[c][1]); }
    }

    /* Demais passos: persegue pelo caminho mais curto (BFS) — navega ao
       redor do covil sem travar, ao contrario do antigo guloso por Manhattan. */
    int pv = coord_to_vertex(m->pacman.x, m->pacman.y);
    bfs(m->maze_graph, pv, m->bfs_dist, m->bfs_prev);
    return ghost_step_by_dist(m, gi, m->bfs_dist);
}

/* Fuga (modo assustado): escolhe o movimento valido que mais AFASTA
   do Pac-Man, evitando dar meia-volta. Movimento proposital — nao fica
   vagando a esmo como o aleatorio puro. */
static int ghost_flee(GameModel *m, int gi) {
    Ghost *g = &m->ghosts[gi];
    int dirs[4][2] = {{0,-1},{0,1},{-1,0},{1,0}};
    int best = DIR_NONE, best_d = -1, reverse_dir = DIR_NONE;
    for (int d = 0; d < 4; d++) {
        int nx = g->x + dirs[d][0];
        int ny = g->y + dirs[d][1];
        if (ny == TUNNEL_ROW) { if (nx < 0) nx = MAP_W-1; else if (nx >= MAP_W) nx = 0; }
        if (!model_is_navigable(m, nx, ny, 1)) continue;
        int dir = step_to_dir(dirs[d][0], dirs[d][1]);
        if (dirs[d][0] == -g->dx && dirs[d][1] == -g->dy) { reverse_dir = dir; continue; }
        int nd = abs(nx - m->pacman.x) + abs(ny - m->pacman.y);
        if (nd > best_d) { best_d = nd; best = dir; }   /* afasta o maximo do Pac-Man */
    }
    return best != DIR_NONE ? best : reverse_dir;       /* so reverte em beco sem saida */
}

/* Despacha a IA correta para cada fantasma */
int model_ghost_ai(GameModel *m, int gi) {
    Ghost *g = &m->ghosts[gi];
    if (g->mode == GMODE_FRIGHTENED)
        return ghost_flee(m, gi);
    if (g->mode == GMODE_EATEN)
        return ghost_random(m, gi);
    switch (g->ai_type) {
        case AI_DIJKSTRA: return ghost_dijkstra(m, gi);
        case AI_BFS:      return ghost_bfs(m, gi);
        case AI_DFS:      return ghost_dfs(m, gi);
        default:          return ghost_random(m, gi);
    }
}

/* ══════════════════════════════════════════════════════════
   AUTO-PLAY — IA que controla o Pac-Man sozinho (modo demo)
   Estrategia (reaproveita o grafo + BFS):
     1. BFS do Pac-Man para todas as celulas.
     2. Escolhe um ALVO: fantasma assustado mais proximo (para
        comer) ou, na falta dele, o ponto/cereja mais proximo.
     3. Mapa de PERIGO: BFS multi-origem dos fantasmas cacadores
        (distancia da celula ao cacador mais proximo).
     4. Pontua as 4 direcoes: avanca rumo ao alvo evitando perigo.
   ══════════════════════════════════════════════════════════ */

/* BFS multi-origem: dist[v] = passos da celula v ao cacador mais proximo */
static void bfs_danger_map(GameModel *m, int *dist) {
    int n = m->maze_graph->num_vertices;
    for (int i = 0; i < n; i++) dist[i] = INF;
    Queue *q = queue_create(n);
    for (int i = 0; i < GHOSTS; i++) {
        Ghost *g = &m->ghosts[i];
        if (!g->active) continue;
        if (g->mode == GMODE_FRIGHTENED || g->mode == GMODE_EATEN) continue;
        int v = coord_to_vertex(g->x, g->y);
        if (dist[v] == INF) { dist[v] = 0; queue_push(q, v); }
    }
    while (!queue_empty(q)) {
        int u = queue_pop(q);
        for (AdjNode *a = m->maze_graph->adj_list[u]; a; a = a->next)
            if (dist[a->vertex] == INF) {
                dist[a->vertex] = dist[u] + 1;
                queue_push(q, a->vertex);
            }
    }
    queue_free(q);
}

/* Decide a melhor direcao do Pac-Man — O(V+E) */
int model_pacman_ai(GameModel *m) {
    PacMan *p = &m->pacman;
    Graph  *g = m->maze_graph;
    if (!g) return DIR_NONE;

    /* 1. BFS a partir do Pac-Man */
    int src = coord_to_vertex(p->x, p->y);
    bfs(g, src, m->bfs_dist, m->bfs_prev);

    /* 3. Mapa de perigo (cacadores) */
    static int danger[MAX_VERTICES];
    bfs_danger_map(m, danger);

    /* 2. Alvo: fantasma assustado mais proximo (prioridade) ... */
    int target = -1, tdist = INF;
    for (int i = 0; i < GHOSTS; i++) {
        Ghost *gh = &m->ghosts[i];
        if (!gh->active || gh->mode != GMODE_FRIGHTENED) continue;
        int v = coord_to_vertex(gh->x, gh->y);
        if (m->bfs_dist[v] < tdist) { tdist = m->bfs_dist[v]; target = v; }
    }
    /* ... senao o ponto/cereja mais proximo */
    if (target == -1) {
        for (int y = 0; y < MAP_H; y++)
            for (int x = 0; x < MAP_W; x++) {
                int c = m->grid[y][x];
                if (c != CELL_DOT && c != CELL_POWER) continue;
                int v = coord_to_vertex(x, y);
                if (m->bfs_dist[v] < tdist) { tdist = m->bfs_dist[v]; target = v; }
            }
    }

    /* Primeiro passo do caminho mais curto src -> target (via bfs_prev) */
    int step = -1;
    if (target != -1 && m->bfs_dist[target] < INF) {
        int cur = target, prev = -1;
        while (cur != -1 && cur != src) { prev = cur; cur = m->bfs_prev[cur]; }
        if (cur == src) step = prev;
    }

    /* 4. Pontua as 4 direcoes navegaveis */
    int dirs[4][2] = {{0,-1},{0,1},{-1,0},{1,0}};
    int dcon[4]    = {DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT};
    int    best = DIR_NONE;
    double best_score = -1e18;

    for (int d = 0; d < 4; d++) {
        int nx = p->x + dirs[d][0];
        int ny = p->y + dirs[d][1];
        if (ny == TUNNEL_ROW) { if (nx < 0) nx = MAP_W-1; else if (nx >= MAP_W) nx = 0; }
        if (!model_is_navigable(m, nx, ny, 0)) continue;
        int nv = coord_to_vertex(nx, ny);

        int dcap = danger[nv]; if (dcap > 8) dcap = 8;
        double score = dcap * 3.0;             /* longe dos cacadores = melhor    */
        if (danger[nv] <= 2) score -= 500.0;   /* nunca entrar quase no cacador   */
        if (nv == step)      score += 50.0;    /* avanca rumo ao alvo             */
        if (dirs[d][0] == p->dx && dirs[d][1] == p->dy) score += 0.5; /* suaviza  */

        if (score > best_score) { best_score = score; best = dcon[d]; }
    }
    return best;
}

/* Aplica o multiplicador do power-up "Pontos Dobrados" (2x) */
static int pts(GameModel *m, int base) {
    return m->double_timer > 0 ? base * 2 : base;
}

/* Registra um popup de pontos flutuante na posicao (x,y) do mapa */
static void set_popup(GameModel *m, int val, int x, int y) {
    m->popup_val   = val;
    m->popup_x     = x;
    m->popup_y     = y;
    m->popup_timer = POPUP_DURATION;
}

/* ══════════════════════════════════════════════════════════
   MOVIMENTO DO PAC-MAN
   ══════════════════════════════════════════════════════════ */
void model_move_pacman(GameModel *m) {
    PacMan *p = &m->pacman;

    /* Auto-play: a IA define a proxima direcao (substitui o teclado) */
    if (m->autoplay) {
        switch (model_pacman_ai(m)) {
            case DIR_UP:    p->next_dx =  0; p->next_dy = -1; break;
            case DIR_DOWN:  p->next_dx =  0; p->next_dy =  1; break;
            case DIR_LEFT:  p->next_dx = -1; p->next_dy =  0; break;
            case DIR_RIGHT: p->next_dx =  1; p->next_dy =  0; break;
            default: break;
        }
    }

    /* Tenta mudar para a direcao enfileirada */
    int ndx = p->next_dx, ndy = p->next_dy;
    if ((ndx || ndy) && model_is_navigable(m, p->x + ndx, p->y + ndy, 0)) {
        p->dx = ndx; p->dy = ndy;
    }

    int nx = p->x + p->dx;
    int ny = p->y + p->dy;

    /* Tunel wrap */
    if (ny == TUNNEL_ROW) {
        if (nx < 0) nx = MAP_W - 1;
        else if (nx >= MAP_W) nx = 0;
    }

    if (!model_is_navigable(m, nx, ny, 0)) return;

    p->x = nx; p->y = ny;

    /* Come pontos / coleta itens */
    int c = m->grid[p->y][p->x];
    if (c == CELL_DOT) {
        m->grid[p->y][p->x] = CELL_EMPTY;
        m->respawn_grid[p->y][p->x] = RESPAWN_DELAY;  /* reaparece depois */
        m->score += pts(m, SCORE_DOT);
        m->dot_count--;
        m->stats.dots_eaten++;
    } else if (c == CELL_POWER) {
        m->grid[p->y][p->x] = CELL_EMPTY;
        m->respawn_grid[p->y][p->x] = RESPAWN_DELAY;  /* cereja reaparece depois */
        m->score += pts(m, SCORE_POWER);
        m->dot_count--;
        m->stats.dots_eaten++;
        m->power_active = POWER_DURATION;
        m->ghost_combo  = 0;   /* reinicia o combo a cada energizador comido */
        /* Insere power-up na AVL */
        m->powerup_tree = avl_insert(m->powerup_tree,
            m->frame_count % 100, "PowerPellet", POWER_DURATION, &m->stats);
        /* Coloca fantasmas em modo assustado */
        for (int i = 0; i < GHOSTS; i++) {
            if (m->ghosts[i].mode != GMODE_EATEN) {
                m->ghosts[i].mode = GMODE_FRIGHTENED;
                m->ghosts[i].frighten_timer = POWER_DURATION;
            }
        }
    }
    /* ── Power-ups especiais ─────────────────────────────────── */
    else if (c == CELL_SPEED) {            /* ⚡ Turbo */
        m->grid[p->y][p->x] = CELL_EMPTY;
        m->speed_item_timer = 0;
        m->speed_timer = SPEED_EFFECT_DUR;
        m->score += pts(m, SCORE_POWERITEM);
        m->powerup_tree = avl_insert(m->powerup_tree, CELL_SPEED, "Turbo",  SPEED_EFFECT_DUR, &m->stats);
    } else if (c == CELL_FREEZE) {         /* ❄ Congela fantasmas */
        m->grid[p->y][p->x] = CELL_EMPTY;
        m->freeze_item_timer = 0;
        m->freeze_timer = FREEZE_EFFECT_DUR;
        m->score += pts(m, SCORE_POWERITEM);
        m->powerup_tree = avl_insert(m->powerup_tree, CELL_FREEZE, "Freeze", FREEZE_EFFECT_DUR, &m->stats);
    } else if (c == CELL_SHIELD) {         /* 🛡 Escudo (absorve 1 hit) */
        m->grid[p->y][p->x] = CELL_EMPTY;
        m->shield_item_timer = 0;
        m->shield_active = 1;
        m->score += pts(m, SCORE_POWERITEM);
        m->powerup_tree = avl_insert(m->powerup_tree, CELL_SHIELD, "Shield", 1, &m->stats);
    } else if (c == CELL_DOUBLE) {         /* ✖2 Pontos dobrados */
        m->grid[p->y][p->x] = CELL_EMPTY;
        m->double_item_timer = 0;
        m->double_timer = DOUBLE_EFFECT_DUR;
        m->score += pts(m, SCORE_POWERITEM);
        m->powerup_tree = avl_insert(m->powerup_tree, CELL_DOUBLE, "Double", DOUBLE_EFFECT_DUR, &m->stats);
    } else if (c == CELL_FRUIT) {          /* 🍓 Fruta bonus */
        m->grid[p->y][p->x] = CELL_EMPTY;
        m->fruit_timer = 0;
        int gain = pts(m, SCORE_FRUIT);
        m->score += gain;
        set_popup(m, gain, p->x, p->y);
    }
}

/* ══════════════════════════════════════════════════════════
   MOVIMENTO DOS FANTASMAS
   ══════════════════════════════════════════════════════════ */

/* Direcao deterministica para SAIR do covil (porta em (13/14, 12)).
   Enquanto dentro, ignora a IA normal e segue ate a saida — evita
   que fantasmas das laterais ou de IA erratica fiquem presos.
   Retorna DIR_NONE quando o fantasma ja esta fora (y <= 11). */
static int ghost_house_exit_dir(Ghost *g) {
    /* So age enquanto o fantasma esta DENTRO da caixa do covil
       (linhas 12-15, colunas 11-16). Fora dela, a IA normal assume —
       senao o fantasma tentaria "subir para sair" em qualquer ponto da
       metade inferior do mapa e ficaria oscilando contra as paredes. */
    if (g->y < 12 || g->y > 15 || g->x < 11 || g->x > 16) return DIR_NONE;
    /* Alinha na coluna da porta (13 ou 14) e entao sobe para sair */
    if (g->x < 13) return DIR_RIGHT;
    if (g->x > 14) return DIR_LEFT;
    return DIR_UP;
}

void model_move_ghosts(GameModel *m) {
    /* ❄ Congelados: nenhum fantasma se move enquanto o timer durar */
    if (m->freeze_timer > 0) return;

    for (int i = 0; i < GHOSTS; i++) {
        Ghost *g = &m->ghosts[i];

        /* Libera fantasmas do covil / faz reaparecer os que foram comidos */
        if (!g->active) {
            if (m->frame_count >= g->release_timer) {
                g->active = 1;
                g->mode   = GMODE_CHASE;   /* fantasma comido volta a cacar */
                g->frighten_timer = 0;
            }
            continue;
        }

        /* Timer de medo + meia-velocidade enquanto assustado (da p/ comer) */
        if (g->mode == GMODE_FRIGHTENED) {
            g->frighten_timer--;
            if (g->frighten_timer <= 0) {
                g->mode = GMODE_CHASE;
            } else if (m->frame_count % 2 == 0) {
                continue;   /* pula 1 frame: anda na metade do ritmo do Pac-Man */
            }
        }

        /* Se ainda esta no covil, vai direto para a saida; senao usa a IA */
        int dir = ghost_house_exit_dir(g);
        if (dir == DIR_NONE) dir = model_ghost_ai(m, i);
        int ddx = 0, ddy = 0;
        switch (dir) {
            case DIR_UP:    ddy = -1; break;
            case DIR_DOWN:  ddy =  1; break;
            case DIR_LEFT:  ddx = -1; break;
            case DIR_RIGHT: ddx =  1; break;
            default: break;
        }

        int nx = g->x + ddx;
        int ny = g->y + ddy;

        /* Tunel wrap */
        if (ny == TUNNEL_ROW) {
            if (nx < 0) nx = MAP_W - 1;
            else if (nx >= MAP_W) nx = 0;
        }

        if (model_is_navigable(m, nx, ny, 1)) {
            g->x = nx; g->y = ny;
            g->dx = ddx; g->dy = ddy;
        } else {
            /* Direcao aleatoria alternativa */
            dir = ghost_random(m, i);
            ddx = 0; ddy = 0;
            switch (dir) {
                case DIR_UP:    ddy = -1; break;
                case DIR_DOWN:  ddy =  1; break;
                case DIR_LEFT:  ddx = -1; break;
                case DIR_RIGHT: ddx =  1; break;
                default: break;
            }
            nx = g->x + ddx; ny = g->y + ddy;
            if (ny == TUNNEL_ROW) {
                if (nx < 0) nx = MAP_W - 1;
                else if (nx >= MAP_W) nx = 0;
            }
            if (model_is_navigable(m, nx, ny, 1)) {
                g->x = nx; g->y = ny;
                g->dx = ddx; g->dy = ddy;
            }
        }
    }
}

/* ══════════════════════════════════════════════════════════
   COLISOES
   ══════════════════════════════════════════════════════════ */

/* Recoloca Pac-Man e fantasmas apos o Pac-Man perder uma vida.
   Evita morte em cadeia (fantasma logo em cima do ponto de respawn). */
static void reset_after_death(GameModel *m) {
    m->pacman.x = 12; m->pacman.y = 23;
    m->pacman.dx = 0; m->pacman.dy = 0;
    m->pacman.next_dx = 0; m->pacman.next_dy = 0;
    m->power_active = 0;

    int grel[4] = {0, 20, 40, 60};
    for (int i = 0; i < GHOSTS; i++) {
        Ghost *g = &m->ghosts[i];
        g->x = g->home_x; g->y = g->home_y;
        g->dx = g->dy = 0;
        g->mode = GMODE_SCATTER;
        g->frighten_timer = 0;
        g->active = (i == 0) ? 1 : 0;
        g->release_timer = m->frame_count + grel[i];
    }
}

/* Resolve a colisao com 1 fantasma. Retorna 1 se o Pac-Man foi pego. */
static int resolve_ghost_collision(GameModel *m, int gi) {
    Ghost *g = &m->ghosts[gi];

    if (g->mode == GMODE_FRIGHTENED) {
        /* Pac-Man come o fantasma vulneravel — combo 200/400/800/1600 */
        int combo = m->ghost_combo < 3 ? m->ghost_combo : 3;
        int gain  = pts(m, SCORE_GHOST << combo);  /* 200,400,800,1600 (x2 se Dobro) */
        m->score += gain;
        m->ghost_combo++;
        set_popup(m, gain, g->x, g->y);
        m->stats.ghosts_eaten++;
        g->mode    = GMODE_EATEN;
        g->x = g->home_x; g->y = g->home_y;
        g->dx = g->dy = 0;
        g->active  = 0;
        g->frighten_timer = 0;
        g->release_timer  = m->frame_count + 120;  /* reaparece depois */
        return 0;
    }
    if (g->mode == GMODE_CHASE || g->mode == GMODE_SCATTER) {
        /* 🛡 Escudo ativo: absorve o golpe, manda o fantasma p/ casa
           e o jogador NAO perde vida (escudo e consumido) */
        if (m->shield_active) {
            m->shield_active = 0;
            g->mode    = GMODE_EATEN;
            g->x = g->home_x; g->y = g->home_y;
            g->dx = g->dy = 0;
            g->active  = 0;
            g->frighten_timer = 0;
            g->release_timer  = m->frame_count + 120;
            return 0;
        }
        /* Pac-Man e pego — perde uma vida */
        m->pacman.lives--;
        if (m->pacman.lives <= 0) m->state = STATE_GAME_OVER;
        else                      reset_after_death(m);
        return 1;
    }
    return 0;  /* fantasma EATEN nao causa dano nem e comido */
}

/* Versao publica (compatibilidade): checa colisoes na mesma celula. */
void model_check_collisions(GameModel *m) {
    for (int i = 0; i < GHOSTS; i++) {
        Ghost *g = &m->ghosts[i];
        if (!g->active) continue;
        if (g->x == m->pacman.x && g->y == m->pacman.y) {
            if (resolve_ghost_collision(m, i)) return;
        }
    }
}

/* ══════════════════════════════════════════════════════════
   VERIFICACAO DE VITORIA
   ══════════════════════════════════════════════════════════ */
void model_check_win(GameModel *m) {
    if (m->dot_count <= 0) {
        m->level++;
        m->state = STATE_WIN;
    }
}

/* ══════════════════════════════════════════════════════════
   ATUALIZACAO DAS ESTATISTICAS (RF03, M03)
   ══════════════════════════════════════════════════════════ */
void model_update_stats(GameModel *m) {
    m->stats.bst_height    = bst_height(m->score_tree);
    m->stats.bst_count     = bst_count(m->score_tree);
    m->stats.bst_min_score = m->score_tree ? bst_min(m->score_tree) : 0;
    m->stats.bst_max_score = m->score_tree ? bst_max(m->score_tree) : 0;
    m->stats.avl_balance   = avl_balance_factor(m->powerup_tree);
}

/* ══════════════════════════════════════════════════════════
   POWER-UPS ESPECIAIS — spawn periodico e contagem de efeitos
   Os 4 itens surgem em posicoes fixas do mapa, ficam visiveis por
   um tempo e somem se nao forem coletados. Reaparecem em ciclos.
   ══════════════════════════════════════════════════════════ */
static void powerup_spawn_one(GameModel *m, int *spawn, int *itemt,
                              int x, int y, int cell, int interval, int mapdur) {
    /* Item ja visivel no mapa: conta o tempo de permanencia */
    if (m->grid[y][x] == cell) {
        if (--(*itemt) <= 0) m->grid[y][x] = CELL_EMPTY;   /* sumiu sem coletar */
        return;
    }
    *itemt = 0;                       /* nao esta no mapa (coletado ou nao surgiu) */
    if (--(*spawn) > 0) return;       /* ainda nao e hora de reaparecer */
    *spawn = interval;
    /* So aparece em celula vazia e sem o Pac-Man por cima */
    if (m->grid[y][x] == CELL_EMPTY && !(m->pacman.x == x && m->pacman.y == y)) {
        m->grid[y][x] = cell;
        *itemt = mapdur;
    }
}

static void model_update_powerups(GameModel *m) {
    /* Decrementa os efeitos ativos (o escudo dura ate absorver 1 golpe) */
    if (m->speed_timer  > 0) m->speed_timer--;
    if (m->freeze_timer > 0) m->freeze_timer--;
    if (m->double_timer > 0) m->double_timer--;

    /* Spawns periodicos dos 4 itens nas posicoes fixas (pacman.h) */
    powerup_spawn_one(m, &m->speed_spawn,  &m->speed_item_timer,
                      SPEED_X,  SPEED_Y,  CELL_SPEED,  SPEED_INTERVAL,  SPEED_MAP_DUR);
    powerup_spawn_one(m, &m->freeze_spawn, &m->freeze_item_timer,
                      FREEZE_X, FREEZE_Y, CELL_FREEZE, FREEZE_INTERVAL, FREEZE_MAP_DUR);
    powerup_spawn_one(m, &m->shield_spawn, &m->shield_item_timer,
                      SHIELD_X, SHIELD_Y, CELL_SHIELD, SHIELD_INTERVAL, SHIELD_MAP_DUR);
    powerup_spawn_one(m, &m->double_spawn, &m->double_item_timer,
                      DOUBLE_X, DOUBLE_Y, CELL_DOUBLE, DOUBLE_INTERVAL, DOUBLE_MAP_DUR);

    /* Fruta bonus: surge periodicamente em (FRUIT_X, FRUIT_Y) */
    powerup_spawn_one(m, &m->fruit_spawn, &m->fruit_timer,
                      FRUIT_X, FRUIT_Y, CELL_FRUIT, FRUIT_INTERVAL, FRUIT_DURATION);

    /* Popup de pontos: decai e sobe lentamente na tela */
    if (m->popup_timer > 0) {
        m->popup_timer--;
        if (m->popup_timer % 8 == 0 && m->popup_y > 0) m->popup_y--;
    }
}

/* ══════════════════════════════════════════════════════════
   REAPARECIMENTO — pontos e cerejas comidos voltam apos
   RESPAWN_DELAY frames, no MESMO lugar onde estavam.
   ══════════════════════════════════════════════════════════ */
static void model_update_respawn(GameModel *m) {
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            if (m->respawn_grid[y][x] <= 0) continue;
            if (--m->respawn_grid[y][x] > 0) continue;   /* ainda contando */

            int orig = m->original_grid[y][x];
            /* Reaparece so se for ponto/cereja, a celula estiver vazia e
               sem o Pac-Man em cima (senao ficaria um ponto intocado sob ele) */
            if ((orig == CELL_DOT || orig == CELL_POWER) &&
                m->grid[y][x] == CELL_EMPTY &&
                !(m->pacman.x == x && m->pacman.y == y)) {
                m->grid[y][x] = orig;
                m->dot_count++;
            } else {
                m->respawn_grid[y][x] = 10;   /* ocupado: tenta de novo em breve */
            }
        }
    }
}

/* ══════════════════════════════════════════════════════════
   UPDATE PRINCIPAL DO MODELO
   ══════════════════════════════════════════════════════════ */
void model_update(GameModel *m) {
    if (m->state != STATE_PLAYING) return;

    m->frame_count++;

    /* Espaca a 1a aparicao de cada power-up (evita os 4 juntos no inicio) */
    if (m->frame_count == 1) {
        m->speed_spawn  = SPEED_INTERVAL / 2;
        m->freeze_spawn = FREEZE_INTERVAL;
        m->shield_spawn = SHIELD_INTERVAL * 2;
        m->double_spawn = DOUBLE_INTERVAL + DOUBLE_INTERVAL / 2;
        m->fruit_spawn  = FRUIT_INTERVAL;
        /* Proximo limiar de vida extra acima do score atual */
        m->next_extra_life = (m->score / EXTRA_LIFE_SCORE + 1) * EXTRA_LIFE_SCORE;
    }

    /* Decrementa power-up ativo */
    if (m->power_active > 0) m->power_active--;

    /* Guarda posicao do Pac-Man ANTES de mover (p/ detectar troca) */
    int pac_ox = m->pacman.x, pac_oy = m->pacman.y;
    model_move_pacman(m);
    int pac_nx = m->pacman.x, pac_ny = m->pacman.y;

    /* (a) Colisao logo apos o Pac-Man mover: ele entrou na celula do fantasma */
    for (int i = 0; i < GHOSTS; i++) {
        Ghost *g = &m->ghosts[i];
        if (!g->active) continue;
        if (g->x == m->pacman.x && g->y == m->pacman.y) {
            if (resolve_ghost_collision(m, i)) {
                if (m->state != STATE_PLAYING) return;     /* game over */
                pac_ox = pac_nx = m->pacman.x;             /* pacman resetado */
                pac_oy = pac_ny = m->pacman.y;
            }
        }
    }

    /* ⚡ Turbo: Pac-Man da um passo EXTRA antes de os fantasmas moverem,
       efetivamente andando a 2x a velocidade deles enquanto durar */
    if (m->speed_timer > 0) {
        pac_ox = m->pacman.x; pac_oy = m->pacman.y;
        model_move_pacman(m);
        pac_nx = m->pacman.x; pac_ny = m->pacman.y;
        for (int i = 0; i < GHOSTS; i++) {
            Ghost *g = &m->ghosts[i];
            if (!g->active) continue;
            if (g->x == m->pacman.x && g->y == m->pacman.y) {
                if (resolve_ghost_collision(m, i)) {
                    if (m->state != STATE_PLAYING) return;
                    pac_ox = pac_nx = m->pacman.x;
                    pac_oy = pac_ny = m->pacman.y;
                }
            }
        }
    }

    /* Guarda posicoes dos fantasmas ANTES de moverem */
    int gox[GHOSTS], goy[GHOSTS];
    for (int i = 0; i < GHOSTS; i++) { gox[i] = m->ghosts[i].x; goy[i] = m->ghosts[i].y; }

    model_move_ghosts(m);

    /* (b) Colisao apos fantasmas moverem — inclui TROCA de posicao (swap),
           que e o caso em que se cruzam sem cair na mesma celula */
    for (int i = 0; i < GHOSTS; i++) {
        Ghost *g = &m->ghosts[i];
        if (!g->active) continue;
        int same = (g->x == m->pacman.x && g->y == m->pacman.y);
        int swap = (g->x == pac_ox && g->y == pac_oy &&
                    gox[i] == pac_nx && goy[i] == pac_ny);
        if (same || swap) {
            if (resolve_ghost_collision(m, i)) {
                if (m->state != STATE_PLAYING) return;     /* game over */
                pac_ox = pac_nx = m->pacman.x;
                pac_oy = pac_ny = m->pacman.y;
            }
        }
    }

    /* Power-ups: timers de efeito + spawn periodico dos itens no mapa */
    model_update_powerups(m);
    /* Reaparecimento de pontos/cerejas comidos */
    model_update_respawn(m);

    /* Vida extra ao cruzar cada limiar de EXTRA_LIFE_SCORE pontos */
    while (m->score >= m->next_extra_life) {
        m->pacman.lives++;
        m->next_extra_life += EXTRA_LIFE_SCORE;
    }

    model_check_win(m);
    model_update_stats(m);

    /* Atualiza distancias do grafo a partir do Pac-Man (a cada 5 frames) */
    if (m->frame_count % 5 == 0) {
        int pv = coord_to_vertex(m->pacman.x, m->pacman.y);
        bfs     (m->maze_graph, pv, m->bfs_dist,     m->bfs_prev);
        dijkstra(m->maze_graph, pv, m->dijkstra_dist, m->dijkstra_prev);
    }
}

/* ══════════════════════════════════════════════════════════
   PERSISTENCIA — P01 / P02
   Formato: [int count][score+name × count]
   ══════════════════════════════════════════════════════════ */
void model_save_scores(GameModel *m) {
    FILE *f = fopen("scores.dat", "wb");
    if (!f) return;

    int arr[MAX_SCORES];
    char names[MAX_SCORES][16];
    int cnt = 0;
    bst_inorder_arr(m->score_tree, arr, names, &cnt);

    fwrite(&cnt, sizeof(int), 1, f);
    for (int i = 0; i < cnt; i++) {
        fwrite(&arr[i],    sizeof(int),   1, f);
        fwrite(names[i],  sizeof(char), 16, f);
    }
    fclose(f);
}

void model_load_scores(GameModel *m) {
    FILE *f = fopen("scores.dat", "rb");
    if (!f) return;

    int cnt = 0;
    fread(&cnt, sizeof(int), 1, f);
    if (cnt > MAX_SCORES) cnt = MAX_SCORES;
    for (int i = 0; i < cnt; i++) {
        int   score = 0;
        char  name[16];
        fread(&score, sizeof(int),   1, f);
        fread(name,   sizeof(char), 16, f);
        m->score_tree = bst_insert(m->score_tree, score, name);
    }
    fclose(f);
}
