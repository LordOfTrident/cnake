BIN = ./bin
OUT = $(BIN)/app
INSTALL_FOLDER = /usr/bin
INSTALL        = $(INSTALL_FOLDER)/cnake

SRC  = $(wildcard src/*.c)
DEPS = $(wildcard src/*.h)
OBJ  = $(addsuffix .o,$(subst src/,$(BIN)/,$(basename $(SRC))))

CSTD = c11
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	CSTD = gnu11
endif

CC     = gcc
CFLAGS = -O2 -std=$(CSTD) -Wall -Wextra -Werror -pedantic -Wno-deprecated-declarations
LIBS   = -lm -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer

$(OUT): $(BIN) $(OBJ) $(SRC)
	cp -r ./res/cnake_assets ./bin/
	$(CC) $(CFLAGS) -o $(OUT) $(OBJ) $(LIBS)

$(BIN)/%.o: src/%.c $(DEPS)
	$(CC) -c $< $(CFLAGS) -o $@

$(BIN):
	mkdir -p $(BIN)

install: $(OUT)
	cp $(OUT) $(INSTALL)
	cp -r ./res/cnake_assets $(INSTALL_FOLDER)/

clean:
	rm -r $(BIN)/*

all:
	@echo compile, install, clean
