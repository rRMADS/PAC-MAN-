# PAC-MAN em C — Terminal

> Projeto acadêmico da disciplina **Estruturas de Dados Aplicadas ao Desenvolvimento de Jogos**  
> ADS — 3.º Período

Implementação completa do clássico Pac-Man rodando no terminal, com cores ANSI, múltiplas IAs de fantasmas e diversas estruturas de dados aplicadas diretamente na mecânica do jogo.

---

## Funcionalidades

- **Mapa clássico 28×31** com tunnel lateral e covil de fantasmas
- **4 fantasmas** com inteligências artificiais distintas:
  - Blinky (vermelho) — Dijkstra
  - Pinky (rosa) — BFS
  - Inky (ciano) — DFS
  - Clyde (laranja) — Aleatório
- **Power-ups especiais** além do clássico energizador:
  - Turbo (velocidade aumentada)
  - Gelo (fantasmas congelam)
  - Escudo (absorve um hit)
  - 2x (pontos dobrados)
- **Fruta bônus** com aparição periódica
- **Combo** de fantasmas (200 / 400 / 800 / 1600 pts)
- **Vida extra** a cada 10.000 pontos
- **Ranking persistente** salvo em arquivo binário (`scores.dat`)
- **Modo Demo (AutoPlay)** — IA controla o Pac-Man automaticamente
- **4 velocidades de jogo**: Lenta, Normal, Rápida, Extrema
- **Modo Debug** (tecla `D`): exibe distâncias calculadas pelos algoritmos de grafo
- **Relatório final** com estatísticas detalhadas ao fim de cada partida

---

## Estruturas de Dados Implementadas

| Estrutura | Uso no Jogo | Complexidade |
|-----------|-------------|--------------|
| **BST** (Árvore Binária de Busca) | Ranking de high scores | O(log n) inserção/busca |
| **AVL** (Árvore Autobalanceada) | Inventário de power-ups ativos | O(log n) garantido |
| **Grafo** (Lista de adjacências + Matriz) | Mapa do labirinto para navegação dos fantasmas | O(V+E) |
| **Fila** | BFS dos fantasmas | O(1) enqueue/dequeue |
| **Min-Heap** | Dijkstra (Blinky) | O(log V) push/pop |

### Algoritmos de Busca em Grafo

- **BFS** — Pinky encontra o caminho mais curto (arestas não ponderadas)
- **DFS** — Inky explora o labirinto em profundidade
- **Dijkstra** — Blinky garante o caminho ótimo com pesos

### 7 Algoritmos de Ordenação

Todos implementados do zero e cronometrados (µs) no ranking:

| Algoritmo | Complexidade |
|-----------|-------------|
| Bubble Sort | O(n²) |
| Selection Sort | O(n²) |
| Insertion Sort | O(n²) |
| Shell Sort | O(n log n) |
| Merge Sort | O(n log n) |
| Quick Sort | O(n log n) |
| Heap Sort | O(n log n) |

---

## Arquitetura

O projeto segue o padrão **MVC**:

```
.
├── main.c              # Ponto de entrada
├── pacman.h            # Interface completa (structs, defines, protótipos)
├── model/
│   └── model.c         # Lógica do jogo, estruturas de dados, IA
├── view/
│   └── view.c          # Renderização no terminal (ANSI)
├── controller/
│   └── controller.c    # Loop principal, input do teclado
├── test.c              # Suite de testes automatizados
├── Makefile
└── scores.dat          # Ranking persistido (gerado em tempo de execução)
```

---

## Compilação e Execução

### Pré-requisitos

- GCC (MinGW-w64 no Windows ou GCC nativo no Linux/macOS)
- Terminal com suporte a ANSI/VT100 (Windows Terminal, WSL, Linux, macOS)

### Compilar e jogar

```bash
make
./pacman.exe
```

### Executar os testes automatizados

```bash
make test
```

### Limpar arquivos compilados

```bash
make clean
```

---

## Controles

| Tecla | Ação |
|-------|------|
| `W` / `↑` | Mover para cima |
| `S` / `↓` | Mover para baixo |
| `A` / `←` | Mover para esquerda |
| `D` / `→` | Mover para direita |
| `P` | Pausar / Retomar |
| `D` (no jogo) | Ativar modo debug |
| `ESC` | Voltar ao menu |

---

## Testes Automatizados

A suite (`test.c`) valida:

- **BST**: inserção, busca, remoção (folha, 1 filho, 2 filhos), altura, min/max, traversal inorder
- **AVL**: autobalanceamento, rotações simples e duplas (LL, RR, LR, RL), fator de balanço
- **7 Sorts**: ordenação correta em arrays aleatórios, já ordenados e reversos
- **Grafo**: matriz de adjacências, BFS (distâncias), Dijkstra (caminho ótimo), DFS (cobertura)

---

## Tecnologias

- **Linguagem**: C11
- **Compilador**: GCC com flags `-Wall -Wextra -O2`
- **Renderização**: Códigos de escape ANSI diretamente no terminal
- **Persistência**: I/O binário em `scores.dat`
- **Plataforma**: Windows (MinGW) / Linux / macOS

---

## Disciplina

Projeto desenvolvido para a disciplina **Estruturas de Dados Aplicadas ao Desenvolvimento de Jogos**  
Curso: Análise e Desenvolvimento de Sistemas (ADS) — 3.º Período
