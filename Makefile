UNAME_S := $(shell uname -s)

PREFIX ?= /usr/local
MANPREFIX ?= $(PREFIX)/share/man

STRIP ?= strip
PKG_CONFIG ?= pkg-config
INSTALL ?= install

CFLAGS_OPTIMIZATION ?= -O3

BUILD = build
BIN   = ffpanel

SRC       = $(wildcard src/*.c)
HEADERS   = $(wildcard src/*.h)

# HEADERS  += $(wildcard src/tui/*.h)
# HEADERS  += $(wildcard src/tui/widgets/*.h)

# SRC      += $(wildcard src/tui/*.c)
# SRC      += $(wildcard src/tui/widgets/*.c)

OUT       = $(SRC:%.c=$(BUILD)/%.o)

CFLAGS += -Isrc -std=c17 -DCOMPILED_TIME_PREFIX='"$(PREFIX)"'

# convert targets to flags for backwards compatibility
O_DEBUG := 0  # debug binary
ifneq ($(filter debug,$(MAKECMDGOALS)),)
	O_DEBUG := 1
endif

ifeq ($(strip $(O_DEBUG)),1)
	CFLAGS += -g3 -DDEBUG -DLOG_SHOW_SOURCE_LOCATION
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

$(BUILD): ## Create build directories automatically
	mkdir -p $(BUILD)

$(BUILD)/%.o: %.c $(SHARED_HDR) $(DAEMON_HDR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(SRC) $(OUT) $(HEADERS) ## Build the ffpanel binary
	$(CC) $(LDFLAGS) -o $@ $(OUT) $(LDLIBS)

debug: $(BIN) ## Build the debug binary run `make debug -B O_DEBUG=1`

install: all ## Install the ffpanel binary
	$(INSTALL) -m 0755 -d $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) -m 0755 $(BIN) $(DESTDIR)$(PREFIX)/bin

clean: ## Clean up build artifacts
	$(RM) -f $(OUT) $(BIN)

uninstall: ## Uninstall the ffpanel binary
	$(RM) $(DESTDIR)$(PREFIX)/bin/$(BIN)

strip: $(BIN) ## Strip the ffpanel binary
	$(STRIP) $^

.PHONY: all install uninstall strip clean
