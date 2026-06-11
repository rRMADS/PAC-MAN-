/*
 * main.c — Ponto de entrada do PAC-MAN em C
 *
 * Inicializa o GameModel e inicia o loop principal
 * via controller_run().
 */

#include "pacman.h"

int main(void) {
    srand((unsigned int)time(NULL));

    GameModel game;
    controller_run(&game);

    return 0;
}
