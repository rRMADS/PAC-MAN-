# ============================================================
# Makefile — PAC-MAN em C
# Compilador: GCC (MinGW-w64 no Windows / GCC no Linux)
# Uso:
#   make          → compila pacman.exe
#   make test     → compila e executa testes (P07)
#   make clean    → remove arquivos compilados
# ============================================================

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2
TARGET  = pacman.exe
TEST_T  = test.exe

SRCS    = main.c \
          model/model.c \
          view/view.c \
          controller/controller.c

OBJS    = $(SRCS:.c=.o)

# ─── Alvo principal ───────────────────────────────────────
all: $(TARGET)
	@echo.
	@echo  [OK] Compilado com sucesso: $(TARGET)
	@echo  Execute: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Regra generica para .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ─── Dependencias de headers ──────────────────────────────
main.o:                  pacman.h
model/model.c:           pacman.h
view/view.c:             pacman.h
controller/controller.c: pacman.h

# ─── Testes (P07) ─────────────────────────────────────────
test: $(TEST_T)
	@echo.
	@echo  === Executando testes ===
	$(TEST_T)

$(TEST_T): test.c model/model.c pacman.h
	$(CC) $(CFLAGS) -o $(TEST_T) test.c model/model.c

# ─── Limpeza ──────────────────────────────────────────────
clean:
	-del /Q *.o 2>NUL
	-del /Q model\*.o 2>NUL
	-del /Q view\*.o 2>NUL
	-del /Q controller\*.o 2>NUL
	-del /Q $(TARGET) $(TEST_T) 2>NUL
	@echo  [OK] Limpeza concluida.

.PHONY: all test clean
