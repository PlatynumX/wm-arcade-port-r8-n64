# Primary target: Nintendo 64 / libdragon. Portable core remains libdragon-free.
ROMNAME := wm_arcade_r9
BUILD_DIR := build/n64

ifeq ($(N64_INST),)
$(error N64_INST is not set. Install/use libdragon or build with its Docker image.)
endif
ifeq ($(wildcard $(N64_INST)/include/n64.mk),)
$(error $(N64_INST)/include/n64.mk not found; N64_INST is incorrect)
endif

include $(N64_INST)/include/n64.mk

CFLAGS += -I$(CURDIR)/include

CORE_C := \
    src/core/process.c \
    src/core/source_clock.c \
    src/core/bmod.c \
    src/core/select.c \
    src/core/anim.c \
    src/core/game.c \
    src/core/ropes.c \
    src/core/demo.c \
    src/core/visual.c \
    src/core/roster.c \
    src/core/attract.c \
    src/core/app.c \
    src/core/composite.c \
    src/generated/attract_sequence.c \
    src/generated/select_tables.c \
    src/generated/port_status.c \
    src/generated/finish_sequences.c \
    src/generated/bret_visuals.c \
    src/generated/bret_attacks.c
ASSET_C := src/generated/bret_sprites.c src/generated/sports_logo.c src/generated/dcs_logo.c src/generated/title_screen.c src/generated/bmod_tables.c
N64_C := src/platform/n64/main.c
C_FILES := $(CORE_C) $(ASSET_C) $(N64_C)
OBJS := $(addprefix $(BUILD_DIR)/,$(C_FILES:.c=.o))

all: $(ROMNAME).z64

assets: $(ASSET_C)

src/generated/bret_sprites.c: tools/wimpimg.py tools/bret_bundle.py tools/bret_manifest.py scripts/prepare_bret_sprites.sh src/generated/bret_visuals.c src/generated/bret_attacks.c
	sh ./scripts/prepare_bret_sprites.sh

src/generated/sports_logo.c: tools/wimpimg.py tools/frontend_bundle.py tools/dcs_bundle.py scripts/prepare_frontend_assets.sh
	sh ./scripts/prepare_frontend_assets.sh

src/generated/dcs_logo.c: src/generated/sports_logo.c
	@test -s $@ || sh ./scripts/prepare_frontend_assets.sh

src/generated/title_screen.c: src/generated/sports_logo.c
	@test -s $@ || sh ./scripts/prepare_frontend_assets.sh

src/generated/bmod_tables.c: src/generated/sports_logo.c tools/bmod_source.py
	@test -s $@ || sh ./scripts/prepare_frontend_assets.sh

$(BUILD_DIR)/$(ROMNAME).elf: $(OBJS)

$(ROMNAME).z64: N64_ROM_TITLE = "WM Arcade Port r9"
$(ROMNAME).z64: N64_ROM_REGIONFREE = true

clean:
	$(RM) -r $(BUILD_DIR) $(ROMNAME).z64 $(ASSET_C)

.PHONY: all clean assets

ifneq ($(wildcard $(BUILD_DIR)),)
-include $(shell find $(BUILD_DIR) -name '*.d')
endif
