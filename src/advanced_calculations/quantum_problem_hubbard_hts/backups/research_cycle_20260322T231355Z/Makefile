CC ?= gcc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -DLUMVORAX_ENABLED=1
LDFLAGS = -Wl,-Bdynamic,--as-needed
LDLIBS ?= -lm -lpthread

LUM_ROOT := ../../
LUM_INC  := -I$(LUM_ROOT)
LUM_SRC  := $(LUM_ROOT)debug/ultra_forensic_logger.c \
	$(LUM_ROOT)debug/memory_tracker.c

BIN_MAIN          := hubbard_hts_runner
BIN_RESEARCH      := hubbard_hts_research_runner
BIN_RESEARCH_ADV  := hubbard_hts_research_runner_advanced_parallel

SRC_MAIN          := src/main.c src/hubbard_hts_module.c
ED_SRC            := src/exact_diagonalization.c
WORM_SRC          := src/worm_mc_bosonic.c
SRC_RESEARCH      := src/hubbard_hts_research_cycle.c $(ED_SRC) $(LUM_SRC)
SRC_RESEARCH_ADV  := src/hubbard_hts_research_cycle_advanced_parallel.c $(ED_SRC) $(WORM_SRC) $(LUM_SRC)

all: $(BIN_MAIN) $(BIN_RESEARCH) $(BIN_RESEARCH_ADV)

$(BIN_MAIN): $(SRC_MAIN)
	$(CC) $(CFLAGS) $(LUM_INC) $(LDFLAGS) -o $@ $(SRC_MAIN) $(LDLIBS)

$(BIN_RESEARCH): $(SRC_RESEARCH)
	$(CC) $(CFLAGS) $(LUM_INC) $(LDFLAGS) -o $@ $(SRC_RESEARCH) $(LDLIBS)

$(BIN_RESEARCH_ADV): $(SRC_RESEARCH_ADV)
	$(CC) $(CFLAGS) $(LUM_INC) $(LDFLAGS) -o $@ $(SRC_RESEARCH_ADV) $(LDLIBS)

clean:
	rm -f $(BIN_MAIN) $(BIN_RESEARCH) $(BIN_RESEARCH_ADV)

.PHONY: all clean
