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

CFLAGS += -I$(CURDIR)/include -I$(CURDIR)/src/fix39 -I$(CURDIR)/src/generated

CORE_C := \
    src/core/process.c \
    src/core/source_clock.c \
    src/core/bmod.c \
    src/core/select.c \
    src/core/select_screen.c \
    src/core/select_continue.c \
    src/core/award.c \
    src/core/pregame.c \
    src/core/anim.c \
    src/core/game.c \
    src/core/ropes.c \
    src/core/demo.c \
    src/core/visual.c \
    src/core/roster.c \
    src/core/attract.c \
    src/core/audio.c \
    src/core/app.c \
    src/core/composite.c \
    src/generated/attract_sequence.c \
    src/generated/select_tables.c \
    src/generated/port_status.c \
    src/generated/finish_sequences.c \
    src/generated/bret_visuals.c \
    src/generated/bret_attacks.c
# BEGIN FIX38 CUMULATIVE ARCADE SOURCE PORTS
FIX38_ARCADE_C := \
    src/core/arcade/wm_arcade_roster.c \
    src/core/arcade/wmania_hiscore_adapter.c \
    src/core/arcade/wmania_hiscore_core.c \
    src/core/arcade/wmania_hiscore_counter.c \
    src/core/arcade/wmania_hiscore_entry.c \
    src/core/arcade/wmania_hiscore_factory.c \
    src/core/arcade/wmania_hiscore_persist.c \
    src/core/arcade/wmania_hiscore_present.c \
    src/core/arcade/wmania_hiscore_special.c \
    src/core/arcade/wmania_hiscore_system.c \
    src/core/arcade/wmania_ring_climb.c \
    src/core/arcade/wmania_ring_out.c \
    src/core/arcade/wmania_rope_command.c \
    src/core/arcade/wmania_rope_runtime.c \
    src/core/arcade/wmania_rope_source_data.c \
    src/core/arcade/wmania_rope_spawn.c
CORE_C += $(FIX38_ARCADE_C)
# END FIX38 CUMULATIVE ARCADE SOURCE PORTS


# BEGIN FIX39 SOURCE-DIRECT MERGE
FIX39_C := \
    src/fix39/wm_arcade_anim_combat.c \
    src/fix39/wm_arcade_attach_anim.c \
    src/fix39/wm_arcade_bam.c \
    src/fix39/wm_arcade_bret.c \
    src/fix39/wm_arcade_bret_tables.c \
    src/fix39/wm_arcade_combat.c \
    src/fix39/wm_arcade_doink.c \
    src/fix39/wm_arcade_drone.c \
    src/fix39/wm_arcade_drone_source_bodies.c \
    src/fix39/wm_arcade_drone_source_ranges.c \
    src/fix39/wm_arcade_drone_source_scripts.c \
    src/fix39/wm_arcade_drone_source_services.c \
    src/fix39/wm_arcade_drone_source_tables.c \
    src/fix39/wm_arcade_fireworks.c \
    src/fix39/wm_arcade_lex.c \
    src/fix39/wm_arcade_matchflow.c \
    src/fix39/wm_arcade_move_dispatch.c \
    src/fix39/wm_arcade_movement.c \
    src/fix39/wm_arcade_confine_grounded.c \
    src/fix39/wm_arcade_confine_full.c \
    src/fix39/wm_arcade_wrestle_target.c \
    src/fix39/wm_arcade_wrestle_input.c \
    src/fix39/wm_arcade_razor.c \
    src/fix39/wm_arcade_razor_tables.c \
    src/fix39/wm_arcade_react.c \
    src/fix39/wm_arcade_react1_core.c \
    src/fix39/wm_arcade_react2_core.c \
    src/fix39/wm_arcade_react3_core.c \
    src/fix39/wm_arcade_react4_core.c \
    src/fix39/wm_arcade_react5_core.c \
    src/fix39/wm_arcade_react6_core.c \
    src/fix39/wm_arcade_react7_core.c \
    src/fix39/wm_arcade_react8_core.c \
    src/fix39/wm_arcade_react9_core.c \
    src/fix39/wm_arcade_shawn.c \
    src/fix39/wm_arcade_source_animation_catalog.c \
    src/fix39/wm_arcade_source_animation_program.c \
    src/fix39/wm_arcade_source_animation_runtime.c \
    src/fix39/wm_arcade_source_attack_frames.c \
    src/fix39/wm_arcade_special.c \
    src/fix39/wm_arcade_story.c \
    src/fix39/wm_arcade_taker.c \
    src/fix39/wm_arcade_target_offsets.c \
    src/fix39/wm_arcade_wimp_frame.c \
    src/fix39/wm_arcade_wrestler_port.c \
    src/fix39/wm_arcade_yoko.c \
    src/fix39/wm_fix39_runtime.c \
    src/fix39/wmania_attract_adapter.c \
    src/fix39/wmania_attract_core.c \
    src/fix39/wmania_attract_data.c \
    src/fix39/wmania_attract_live.c \
    src/fix39/wmania_attract_operator.c \
    src/fix39/wmania_attract_secret.c \
    src/fix39/wmania_attract_time.c \
    src/fix39/wmania_attract_visuals.c \
    src/fix39/wmania_ring_geometry.c \
    src/fix39/wmania_ring_onscreen.c \
    src/fix39/wmania_rng.c
# END FIX39 SOURCE-DIRECT MERGE
ASSET_C := src/generated/bret_sprites.c src/generated/character_assets.c src/generated/ring_rope_assets.c src/generated/ring_arena_assets.c src/generated/crowd_assets.c src/generated/sports_logo.c src/generated/dcs_logo.c src/generated/title_screen.c src/generated/title_sparkle.c src/generated/bmod_tables.c src/generated/sports_background.c src/generated/sports_motto.c src/generated/select_sprites.c src/generated/select_background_main.c src/generated/select_background_choice.c src/generated/progress_background.c src/generated/progress_wrestlers.c src/generated/fix39_attract_text_generated.c src/generated/fix39_attract_assets.c
N64_C := src/platform/n64/main.c src/platform/n64/dcs_effect.c src/platform/n64/audio_backend.c src/platform/n64/dcs_bank.c
C_FILES := $(FIX39_C) $(CORE_C) $(ASSET_C) $(N64_C)
OBJS := $(addprefix $(BUILD_DIR)/,$(C_FILES:.c=.o))

all: $(ROMNAME).z64

assets: $(ASSET_C)

src/generated/bret_sprites.c: tools/wimpimg.py tools/bret_bundle.py tools/bret_manifest.py scripts/prepare_bret_sprites.sh src/generated/bret_visuals.c src/generated/bret_attacks.c
	sh ./scripts/prepare_bret_sprites.sh

src/generated/sports_logo.c: tools/wimpimg.py tools/frontend_bundle.py tools/dcs_bundle.py tools/sparkle_bundle.py scripts/prepare_frontend_assets.sh
	sh ./scripts/prepare_frontend_assets.sh

src/generated/dcs_logo.c: src/generated/sports_logo.c
	@test -s $@ || sh ./scripts/prepare_frontend_assets.sh

src/generated/title_screen.c: src/generated/sports_logo.c tools/bdd_bundle.py tools/bmod_source.py
	@test -s $@ || sh ./scripts/prepare_frontend_assets.sh

src/generated/title_sparkle.c: src/generated/sports_logo.c tools/sparkle_bundle.py
	@test -s $@ || sh ./scripts/prepare_frontend_assets.sh

src/generated/bmod_tables.c: src/generated/sports_logo.c tools/bmod_source.py
	@test -s $@ || sh ./scripts/prepare_frontend_assets.sh

src/generated/sports_background.c src/generated/sports_motto.c: tools/wimpimg.py tools/sports_background_bundle.py tools/sports_motto_bundle.py scripts/prepare_sports_source_assets.sh
	sh ./scripts/prepare_sports_source_assets.sh

# BEGIN EXACT WWF DCS COMMAND 1005
# Native 31,250 Hz mono signed-16 output decoded from DCS command 1005/0x03ED.
# Compression 0 is intentional: do not add another lossy codec after the DCS decode.
DCS1005_WAV := assets/dcs/wwf_dcs_cmd1005_31250hz_ref.wav
DCS1005_WAV64 := filesystem/dcs/wwf_dcs_cmd1005_31250hz_ref.wav64

$(DCS1005_WAV64): $(DCS1005_WAV)
	@mkdir -p $(dir $@)
	@echo " [DCS AUDIO] $@"
	@$(N64_AUDIOCONV) --wav-compress 0 --wav-loop false -o $(dir $@) "$<"

$(BUILD_DIR)/$(ROMNAME).dfs: $(DCS1005_WAV64)
# BEGIN FIX39 STREAMED CHARACTER ART
FIX39_CHAR_DFS_FILES := $(wildcard filesystem/fix39_chars/*/*.bin)
$(BUILD_DIR)/$(ROMNAME).dfs: $(FIX39_CHAR_DFS_FILES)
$(ROMNAME).z64: $(BUILD_DIR)/$(ROMNAME).dfs
# END FIX39 STREAMED CHARACTER ART

# BEGIN FIX39 STREAMED ANIM VM
FIX39_ANIM_DFS_FILES := $(wildcard filesystem/fix39_anim/programs/*.bin) $(wildcard filesystem/fix39_anim/tables/*.bin)
$(BUILD_DIR)/$(ROMNAME).dfs: $(FIX39_ANIM_DFS_FILES)
$(ROMNAME).z64: $(BUILD_DIR)/$(ROMNAME).dfs
# END FIX39 STREAMED ANIM VM
# END EXACT WWF DCS COMMAND 1005

# BEGIN EXACT WWF DCS FRONTEND SELECT BANK
DCS_BANK_CMDS := 0176 0208 0244 0248 0460 1376 1456 1460 1512 1556 1560 1564 1568 2560 2564 2568 2572 2576 3640 3644 3648 3652 3656 3660 3664 3668
DCS_BANK_WAVS := $(addprefix assets/dcs/cmd_,$(addsuffix .wav,$(DCS_BANK_CMDS)))
DCS_BANK_WAV64 := $(addprefix filesystem/dcs/cmd_,$(addsuffix .wav64,$(DCS_BANK_CMDS)))

filesystem/dcs/cmd_%.wav64: assets/dcs/cmd_%.wav
	@mkdir -p $(dir $@)
	@echo " [DCS BANK] $@"
	@$(N64_AUDIOCONV) --wav-compress 0 --wav-loop false -o $(dir $@) $<

$(BUILD_DIR)/$(ROMNAME).dfs: $(DCS_BANK_WAV64)
$(ROMNAME).z64: $(BUILD_DIR)/$(ROMNAME).dfs
# END EXACT WWF DCS FRONTEND SELECT BANK

# BEGIN SOURCE CHARACTER SELECT ASSETS
# Generated SELECT/PREGAME C is checked in so CI builds never require arcade ROMs.
# Regeneration is explicit and local only.
.PHONY: prepare-select-assets
prepare-select-assets:
	@test -n "$(WWFMANIA_ZIP)" || (echo "Set WWFMANIA_ZIP=/path/to/wwfmania.zip" >&2; exit 2)
	WWFMANIA_ZIP="$(WWFMANIA_ZIP)" sh ./scripts/prepare_select_assets.sh
# END SOURCE CHARACTER SELECT ASSETS


# BEGIN FIX39 ATTRACT SOURCE TEXT
FIX39_ATTRACT_ASM := original/wwf-wrestlemania/ATTRACT.ASM
FIX39_ATTRACT_TEXT_C := src/generated/fix39_attract_text_generated.c
FIX39_ATTRACT_TEXT_H := src/generated/fix39_attract_text_generated.h
$(FIX39_ATTRACT_TEXT_C) $(FIX39_ATTRACT_TEXT_H): $(FIX39_ATTRACT_ASM) tools/fix39_attract_text.py
	python3 tools/fix39_attract_text.py --source $(FIX39_ATTRACT_ASM) --out-c $(FIX39_ATTRACT_TEXT_C) --out-h $(FIX39_ATTRACT_TEXT_H)
# END FIX39 ATTRACT SOURCE TEXT


# BEGIN FIX39 ATTRACT SOURCE ASSETS
FIX39_ATTRACT_IMG_DIR := original/wwf-wrestlemania/IMG
FIX39_ATTRACT_IMGPAL := original/wwf-wrestlemania/IMGPAL.ASM
FIX39_ATTRACT_ASSET_C := src/generated/fix39_attract_assets.c
FIX39_ATTRACT_ASSET_H := src/generated/fix39_attract_assets_generated.h
$(FIX39_ATTRACT_ASSET_C) $(FIX39_ATTRACT_ASSET_H): tools/fix39_attract_assets.py tools/wimpimg.py
	@test -f $(FIX39_ATTRACT_IMGPAL) || sh scripts/fetch_original.sh
	python3 tools/fix39_attract_assets.py --img-dir $(FIX39_ATTRACT_IMG_DIR) --imgpal $(FIX39_ATTRACT_IMGPAL) --wimpimg tools/wimpimg.py --out-c $(FIX39_ATTRACT_ASSET_C) --out-h $(FIX39_ATTRACT_ASSET_H)
# END FIX39 ATTRACT SOURCE ASSETS


# BEGIN FIX39 ATTRACT GENERATED HEADER ORDER
$(BUILD_DIR)/src/platform/n64/main.o: $(FIX39_ATTRACT_TEXT_H) $(FIX39_ATTRACT_ASSET_H)
# END FIX39 ATTRACT GENERATED HEADER ORDER


# BEGIN FIX39 BRET SOURCE ATTACK FRAMES
FIX39_HRTSEQ2 := original/wwf-wrestlemania/HRTSEQ2.ASM
FIX39_BRET_ATTACK_H := src/fix39/wm_arcade_bret_attack_frames_generated.h
$(FIX39_BRET_ATTACK_H): $(FIX39_HRTSEQ2) tools/fix39_bret_attack_frames.py
	python3 tools/fix39_bret_attack_frames.py --source $(FIX39_HRTSEQ2) --out $(FIX39_BRET_ATTACK_H)
$(BUILD_DIR)/src/fix39/wm_arcade_source_attack_frames.o: $(FIX39_BRET_ATTACK_H)
# END FIX39 BRET SOURCE ATTACK FRAMES

$(BUILD_DIR)/$(ROMNAME).elf: $(OBJS)

$(ROMNAME).z64: N64_ROM_TITLE = "WM Arcade Port r9"
$(ROMNAME).z64: N64_ROM_REGIONFREE = true

clean:
	$(RM) -r $(BUILD_DIR) $(ROMNAME).z64 $(ASSET_C) $(DCS1005_WAV64)

.PHONY: all clean assets

ifneq ($(wildcard $(BUILD_DIR)),)
-include $(shell find $(BUILD_DIR) -name '*.d')
endif
