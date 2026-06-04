UNAME_S := $(shell uname -s)

PREFIX ?= /usr/local
MANPREFIX ?= $(PREFIX)/share/man

STRIP ?= strip
PKG_CONFIG ?= pkg-config
INSTALL ?= install

CFLAGS_OPTIMIZATION ?= -O3

DISTFILES = src Makefile README.md LICENSE

SRC       = $(wildcard src/*.c)
HEADERS   = $(wildcard src/*.h)
# SRC      += $(wildcard src/tui/*.c)
# HEADERS  += $(wildcard src/tui/*.h)
# SRC      += $(wildcard src/tui/widgets/*.c)
# HEADERS  += $(wildcard src/tui/widgets/*.h)

OUT       = $(SRC:.c=.o)
BIN       = ffpanel

CFLAGS += -Isrc -std=c17

# THIRD_PARTY_LIBS = libcjson

# convert targets to flags for backwards compatibility
O_DEBUG := 0  # debug binary
ifneq ($(filter debug,$(MAKECMDGOALS)),)
	O_DEBUG := 1
endif

ifeq ($(strip $(O_DEBUG)),1)
	CFLAGS += -g3 -DDEBUG
else
	CFLAGS += $(CFLAGS_OPTIMIZATION)
endif

# Check if the OS is macOS
ifeq ($(UNAME_S),Darwin)
    LDLIBS += -largp
else # Else every thing is linux
    CFLAGS += -D_GNU_SOURCE
endif


# ── cJSON ─────────────────────────────────────────────────────────────────
ifeq ($(shell $(PKG_CONFIG) --exists libcjson && echo 1),1)
    CFLAGS  += $(shell $(PKG_CONFIG) --cflags libcjson)
    LDFLAGS += $(shell $(PKG_CONFIG) --libs   libcjson)
else
    $(error "libcjson not found. Install it via your package manager.")
endif

# ── ncurses (prefer wide-char build) ──────────────────────────────────────
ifeq ($(shell $(PKG_CONFIG) --exists ncursesw && echo 1),1)
    CFLAGS  += $(shell $(PKG_CONFIG) --cflags ncursesw)
    LDFLAGS += $(shell $(PKG_CONFIG) --libs   ncursesw)
else ifeq ($(shell $(PKG_CONFIG) --exists ncurses && echo 1),1)
    CFLAGS  += $(shell $(PKG_CONFIG) --cflags ncurses)
    LDFLAGS += $(shell $(PKG_CONFIG) --libs   ncurses)
else
    $(warning "ncurses not found via pkg-config, falling back to -lncurses")
    LDFLAGS += -lncurses
endif


all: $(BIN)

help: ## Show this help
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[33m%-20s\033[0m %s\n", $$1, $$2}'

$(BIN): $(SRC) $(OUT) $(HEADERS) ## Build the live-server binary
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OUT) $(LDLIBS)

debug: $(BIN) ## Build the debug binary run `make O_DEBUG=1`

install: all ## Install the live-server binary
	$(INSTALL) -m 0755 -d $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) -m 0755 $(BIN) $(DESTDIR)$(PREFIX)/bin

clean: ## Clean up build artifacts
	$(RM) -f $(OUT) $(BIN)

uninstall: ## Uninstall the live-server binary
	$(RM) $(DESTDIR)$(PREFIX)/bin/$(BIN)

strip: $(BIN) ## Strip the live-server binary
	$(STRIP) $^

.PHONY: all install uninstall strip clean
