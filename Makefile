CC ?= cc
# Optional untracked settings, e.g. SNIPER_ZIP = /path/to/sniper87.zip
-include local.mk
# Original game assets are not in this repository. `make assets SNIPER_ZIP=...`
# generates them into build/assets/ from your own copy of the game.
ASSET_OUTPUTS := build/assets/sniper_assets.c build/assets/sniper_assets.rc build/assets/files/LOCAL.LVL build/assets/files/45538382.LVL
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror
.PHONY: test clean native-test app linux native-linux critical-parity-test critical-numeric-test assets
assets: build/assets/sniper_assets.c
$(ASSET_OUTPUTS) &: tools/extract_assets.py
	@test -n "$(SNIPER_ZIP)" || { echo "set SNIPER_ZIP=/path/to/sniper87.zip (your own copy of the game)" >&2; exit 1; }
	python3 tools/extract_assets.py --zip "$(SNIPER_ZIP)" --out build/assets
app: assets
	$(MAKE) -C platform
native-test critical-parity-test: assets

linux:
	$(MAKE) -C linux

native-linux: assets
	$(MAKE) -C linux -f native-linux.mk

# Persistent differential slices. Requires the generated assets and pefile/unicorn.
critical-parity-test:
	python3 tests/critical_rng_native_test.py
	python3 tests/critical_combat_native_test.py
	python3 tests/critical_frame_native_test.py
	python3 tests/critical_pickups_native_test.py
	python3 tests/critical_capabilities_native_test.py
	python3 tests/critical_network_native_test.py
	python3 tests/critical_network_native_test.py --team
	python3 tests/critical_network_native_test.py --projectiles
	python3 tests/critical_network_death_native_test.py
	python3 tests/critical_network_join_native_test.py
	python3 tests/critical_network_lethal_native_test.py

# Needs the maintainer's research tree (RESEARCH=..., not part of this repository).
critical-numeric-test:
	@test -n "$(RESEARCH)" || { echo "critical-numeric-test needs RESEARCH=/path/to/research" >&2; exit 1; }
	bash $(RESEARCH)/experiments/critical-parity/numeric/run.sh
	$(CC) $(CFLAGS) -Isrc -fPIC -shared $(RESEARCH)/experiments/critical-parity/numeric/host_numeric_bridge.c src/flight_math.c -o $(RESEARCH)/experiments/critical-parity/numeric/build/libnumeric.so
	python3 $(RESEARCH)/experiments/critical-parity/numeric/unicorn_corpus.py
build/level-test: src/level.c src/level.h tests/level_test.c
	mkdir -p build
	$(CC) $(CFLAGS) -Isrc src/level.c tests/level_test.c -o $@
test: build/level-test build/assets/files/LOCAL.LVL
	./build/level-test build/assets/files/LOCAL.LVL build/assets/files/45538382.LVL
clean:
	rm -rf build
build/libmap.so: src/map.c src/map.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/map.c -o $@
native-test: build/libeditor_render.so build/libspectator_control.so build/libchat_history.so build/libchat_input.so build/libchat_render.so build/libchat_dispatch.so build/libradar_markers.so build/libradar_map.so build/liblist_mouse.so build/libstatistics_panel.so build/libhelp_panel.so build/libtext_panel.so build/liblist_navigation.so build/libattachment_command_test.so build/libmanual_warp_test.so build/libcapability_input.so build/libexit_request.so build/libship_flight.so build/libactor_layers.so build/libship_draw_order.so build/libposition_dispatch.so build/libweapon_input.so build/libweapon_action.so build/libweapon_create.so build/liboffline_combat_test.so build/libprojectile_frame.so build/libprojectile_draw.so build/libprojectile_create.so build/libdeath_event.so build/libship_damage.so build/libprojectile_explosion.so build/libprojectile_update.so build/libcamera.so build/libnotices.so build/libattachment.so build/libship_update_order.so build/libpickup_draw.so build/libpickup_sequence_test.so build/libpickup_lifecycle.so build/liboffline_generation.so build/liboffline_pickups_test.so build/libprize_world_test.so build/libprize.so build/libpickup_collect.so build/libship_world_test.so build/libship_update.so build/libframe_clock.so build/libmap.so build/libsprite.so build/librender.so build/liboptions.so build/libship_energy.so build/libmovement.so build/libflight_input.so build/libflight_math.so build/libflight.so build/libship_reset.so build/libship_spawn.so build/liblocal_config.so build/libship_create.so build/libship_effect.so build/libship_effect_draw.so build/libship_body_draw.so build/libship_visual.so build/libship_assets.so build/libship_labels.so build/libship_render.so build/libfont.so build/librender_commands.so build/libpalette.so
	python3 tests/player_capacity_native_test.py
	python3 tests/connection_dialog_native_test.py
	python3 tests/aux_dialog_native_test.py
	python3 tests/score_dialog_native_test.py
	python3 tests/voice_input_native_test.py
	python3 tests/squad_aggregate_native_test.py
	python3 tests/editor_border_native_test.py
	python3 tests/sprite_recovery_native_test.py
	python3 tests/voice_recordings_native_test.py
	python3 tests/audio_activity_native_test.py
	python3 tests/audio_lifecycle_native_test.py
	python3 tests/startup_random_native_test.py
	python3 tests/menu_native_test.py
	python3 tests/reentry_lifetime_native_test.py
	python3 tests/ship_world_variable_clock_native_test.py
	python3 tests/display_input_native_test.py
	python3 tests/network_send_native_test.py
	python3 tests/network_receive_native_test.py
	python3 tests/network_transport_native_test.py
	python3 tests/network_compression_native_test.py
	python3 tests/network_checksum_native_test.py
	python3 tests/network_lifecycle_native_test.py
	python3 tests/network_login_native_test.py
	python3 tests/network_game_packets_native_test.py
	python3 tests/network_game_receive_native_test.py
	python3 tests/network_position_native_test.py
	python3 tests/network_commands_native_test.py
	python3 tests/audio_wave_native_test.py
	python3 tests/chat_lifetime_native_test.py
	python3 tests/inactive_lifecycle_native_test.py
	python3 tests/idle_bitmap_native_test.py
	python3 tests/flight_native_test.py --audio
	python3 tests/bitmap_override_native_test.py
	python3 tests/audio_logic_native_test.py
	python3 tests/spectator_control_native_test.py
	python3 tests/chat_history_native_test.py
	python3 tests/editor_input_native_test.py
	python3 tests/editor_records_native_test.py
	python3 tests/editor_render_native_test.py
	python3 tests/macro_dialog_native_test.py
	python3 tests/chat_input_native_test.py
	python3 tests/chat_render_native_test.py
	python3 tests/chat_dispatch_native_test.py
	python3 tests/radar_markers_native_test.py
	python3 tests/radar_map_native_test.py
	python3 tests/list_mouse_native_test.py
	python3 tests/statistics_panel_native_test.py
	python3 tests/help_panel_native_test.py
	python3 tests/text_panel_native_test.py
	python3 tests/list_navigation_native_test.py
	python3 tests/attachment_command_native_test.py
	python3 tests/manual_warp_native_test.py
	python3 tests/capability_input_native_test.py
	python3 tests/exit_request_native_test.py
	python3 tests/offline_start_native_test.py
	python3 tests/offline_init_native_test.py
	python3 tests/player_registry_native_test.py
	python3 tests/player_team_native_test.py
	python3 tests/player_team_native_test.py --offline
	python3 tests/frame_update_native_test.py
	python3 tests/local_control_native_test.py
	python3 tests/display_recovery_native_test.py
	python3 tests/scene_frame_native_test.py
	python3 tests/hud_panel_native_test.py
	python3 tests/equipment_panel_native_test.py
	python3 tests/surface_fill_native_test.py
	python3 tests/hud_status_native_test.py
	python3 tests/starfield_native_test.py
	python3 tests/actor_layers_native_test.py
	python3 tests/ship_draw_order_native_test.py
	python3 tests/position_dispatch_native_test.py
	python3 tests/weapon_input_native_test.py
	python3 tests/weapon_action_native_test.py
	python3 tests/weapon_action_native_test.py --dispatch
	python3 tests/weapon_action_native_test.py --input
	python3 tests/weapon_action_native_test.py --ordinary
	python3 tests/weapon_create_native_test.py
	python3 tests/offline_combat_native_test.py
	python3 tests/offline_combat_native_test.py --weapon
	python3 tests/offline_combat_native_test.py --action
	python3 tests/offline_combat_native_test.py --input
	python3 tests/offline_combat_native_test.py --control
	python3 tests/offline_combat_native_test.py --frame
	python3 tests/offline_combat_native_test.py --generate
	python3 tests/offline_combat_native_test.py --simulation
	python3 tests/offline_combat_native_test.py --pickups
	python3 tests/projectile_frame_native_test.py
	python3 tests/projectile_update_native_test.py --frame
	python3 tests/projectile_draw_native_test.py
	python3 tests/projectile_create_native_test.py
	python3 tests/death_event_native_test.py
	python3 tests/death_event_native_test.py --real-damage
	python3 tests/ship_damage_native_test.py
	python3 tests/projectile_explosion_native_test.py
	python3 tests/projectile_explosion_native_test.py --real-damage
	python3 tests/projectile_explosion_native_test.py --real-damage --real-create
	python3 tests/projectile_update_native_test.py
	python3 tests/camera_native_test.py
	python3 tests/notices_native_test.py
	python3 tests/attachment_native_test.py
	python3 tests/ship_update_order_native_test.py
	python3 tests/pickup_draw_native_test.py
	python3 tests/pickup_sequence_native_test.py
	python3 tests/pickup_lifecycle_native_test.py
	python3 tests/offline_generation_native_test.py
	python3 tests/offline_pickups_native_test.py
	python3 tests/prize_world_native_test.py
	python3 tests/prize_native_test.py
	python3 tests/pickup_collect_native_test.py
	python3 tests/ship_world_native_test.py
	python3 tests/ship_world_all_native_test.py
	python3 tests/ship_world_offline_native_test.py
	python3 tests/ship_update_native_test.py
	python3 tests/frame_clock_native_test.py
	python3 tests/frame_clock_lifetime_native_test.py
	python3 tests/map_native_test.py
	python3 tests/sprite_native_test.py
	python3 tests/map_render_native_test.py
	python3 tests/options_native_test.py
	python3 tests/ship_energy_native_test.py
	python3 tests/movement_native_test.py
	python3 tests/movement_mutation_native_test.py
	python3 tests/flight_input_native_test.py
	python3 tests/flight_math_native_test.py
	python3 tests/flight_native_test.py
	python3 tests/flight_native_test.py --ship
	python3 tests/flight_native_test.py --ship --poll
	python3 tests/ship_reset_native_test.py
	python3 tests/ship_spawn_native_test.py
	python3 tests/local_config_native_test.py
	python3 tests/ship_create_native_test.py
	python3 tests/ship_effect_native_test.py
	python3 tests/ship_effect_draw_native_test.py
	python3 tests/ship_body_draw_native_test.py
	python3 tests/ship_visual_native_test.py
	python3 tests/ship_assets_native_test.py
	python3 tests/ship_labels_native_test.py
	python3 tests/ship_render_native_test.py
	python3 tests/font_native_test.py
	python3 tests/render_commands_test.py
	python3 tests/palette_native_test.py
build/libsprite.so: src/sprite.c src/sprite.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/sprite.c -o $@
build/librender.so: src/map_render.c src/map_render.h src/sprite.c src/sprite.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/map_render.c src/sprite.c -o $@
build/liboptions.so: src/options.c src/options.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/options.c -o $@

build/libship_energy.so: src/ship_energy.c src/ship_energy.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/ship_energy.c -o $@

build/libmovement.so: src/movement.c src/movement.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/movement.c -o $@

build/libflight_input.so: src/flight_input.c src/flight_input.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/flight_input.c -o $@

build/libflight_math.so: src/flight_math.c src/flight_math.h src/numeric_environment.h tests/flight_math_bridge.c
	mkdir -p build
	$(CC) $(CFLAGS) -Isrc -fPIC -shared src/flight_math.c tests/flight_math_bridge.c -o $@

build/libflight.so: src/flight.c src/flight.h src/flight_boost.c src/flight_boost.h src/flight_input.c src/flight_input.h src/flight_math.c src/flight_math.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/flight.c src/flight_boost.c src/flight_input.c src/flight_math.c -o $@

build/libship_reset.so: src/ship_reset.c src/ship_reset.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/ship_reset.c -o $@

build/libship_spawn.so: src/ship_spawn.c src/ship_spawn.h src/ship_reset.c src/ship_reset.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/ship_spawn.c src/ship_reset.c -o $@

build/liblocal_config.so: src/local_config.c src/local_config.h src/ship_reset.h src/flight_boost.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/local_config.c -o $@

build/libship_create.so: src/ship_create.c src/ship_create.h src/ship_reset.c src/ship_reset.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/ship_create.c src/ship_reset.c -o $@

build/libship_effect.so: src/ship_effect.c src/ship_effect.h src/ship_create.h src/flight_math.c src/flight_math.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/ship_effect.c src/flight_math.c -o $@

build/libship_effect_draw.so: src/ship_effect_draw.c src/ship_effect_draw.h src/ship_create.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/ship_effect_draw.c -o $@

build/libship_body_draw.so: src/ship_body_draw.c src/ship_body_draw.h src/ship_create.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/ship_body_draw.c -o $@

build/libship_visual.so: src/ship_visual.c src/ship_visual.h src/ship_body_draw.c src/ship_body_draw.h src/ship_effect_draw.c src/ship_effect_draw.h src/ship_create.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/ship_visual.c src/ship_body_draw.c src/ship_effect_draw.c -o $@

build/libship_assets.so: src/ship_assets.c src/ship_assets.h src/sprite.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/ship_assets.c -o $@

build/libship_labels.so: src/ship_labels.c src/ship_labels.h src/ship_create.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/ship_labels.c -o $@

build/libship_render.so: src/ship_render.c src/ship_render.h src/ship_visual.c src/ship_visual.h src/ship_labels.c src/ship_labels.h src/ship_body_draw.c src/ship_body_draw.h src/ship_effect_draw.c src/ship_effect_draw.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/ship_render.c src/ship_visual.c src/ship_labels.c src/ship_body_draw.c src/ship_effect_draw.c -o $@

build/libfont.so: src/font.c src/font.h src/sprite.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/font.c -o $@

build/librender_commands.so: src/render_commands.c src/render_commands.h src/ship_assets.c src/font.c src/sprite.c
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/render_commands.c src/ship_assets.c src/font.c src/sprite.c -o $@

build/libpalette.so: src/palette.c src/palette.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/palette.c -o $@

build/libframe_clock.so: src/frame_clock.c src/frame_clock.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/frame_clock.c -o $@

build/libship_update.so: src/ship_update.c src/ship_update.h src/ship_energy.c src/ship_energy.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/ship_update.c src/ship_energy.c -o $@

SHIP_WORLD_SOURCES = src/notices.c src/attachment.c src/ship_update_order.c src/prize.c src/ship_world.c src/ship_update.c src/ship_energy.c src/ship_spawn.c src/ship_reset.c src/movement.c src/local_config.c
build/libship_world_test.so: $(SHIP_WORLD_SOURCES) $(wildcard src/*.h) tests/ship_world_bridge.c
	mkdir -p build
	$(CC) $(CFLAGS) -Isrc -fPIC -shared $(SHIP_WORLD_SOURCES) tests/ship_world_bridge.c -o $@

build/libpickup_collect.so: src/pickup_collect.c src/pickup_collect.h src/ship_create.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/pickup_collect.c -o $@

build/libprize.so: src/prize.c src/prize.h src/local_config.c src/local_config.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/prize.c src/local_config.c -o $@

build/libprize_world_test.so: $(SHIP_WORLD_SOURCES) $(wildcard src/*.h) tests/prize_world_bridge.c
	mkdir -p build
	$(CC) $(CFLAGS) -Isrc -fPIC -shared $(SHIP_WORLD_SOURCES) tests/prize_world_bridge.c -o $@

build/liboffline_pickups_test.so: $(SHIP_WORLD_SOURCES) src/offline_pickups.c src/pickup_collect.c src/pickup_lifecycle.c src/offline_generation.c $(wildcard src/*.h) tests/offline_pickups_bridge.c
	mkdir -p build
	$(CC) $(CFLAGS) -Isrc -fPIC -shared $(SHIP_WORLD_SOURCES) src/offline_pickups.c src/pickup_collect.c src/pickup_lifecycle.c src/offline_generation.c tests/offline_pickups_bridge.c -o $@

build/liboffline_generation.so: src/offline_generation.c src/offline_generation.h src/ship_spawn.c src/ship_reset.c
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/offline_generation.c src/ship_spawn.c src/ship_reset.c -o $@

build/libpickup_lifecycle.so: src/pickup_lifecycle.c src/pickup_lifecycle.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/pickup_lifecycle.c -o $@

build/libpickup_sequence_test.so: $(SHIP_WORLD_SOURCES) src/offline_pickups.c src/pickup_collect.c src/pickup_lifecycle.c src/offline_generation.c $(wildcard src/*.h) tests/pickup_sequence_bridge.c
	mkdir -p build
	$(CC) $(CFLAGS) -Isrc -fPIC -shared $(SHIP_WORLD_SOURCES) src/offline_pickups.c src/pickup_collect.c src/pickup_lifecycle.c src/offline_generation.c tests/pickup_sequence_bridge.c -o $@

build/libpickup_draw.so: src/pickup_draw.c src/pickup_draw.h src/sprite.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/pickup_draw.c -o $@

build/libship_update_order.so: src/ship_update_order.c src/ship_update_order.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/ship_update_order.c -o $@

build/libattachment.so: src/attachment.c src/attachment.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/attachment.c -o $@

build/libnotices.so: src/notices.c src/notices.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/notices.c -o $@

build/libcamera.so: src/camera.c src/camera.h src/ship_spawn.c src/ship_reset.c
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/camera.c src/ship_spawn.c src/ship_reset.c -o $@

build/libprojectile_update.so: src/projectile_update.c src/projectile_update.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/projectile_update.c -o $@

build/libprojectile_explosion.so: src/projectile_explosion.c src/projectile_explosion.h src/projectile_update.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/projectile_explosion.c -o $@

build/libship_damage.so: src/ship_damage.c src/ship_damage.h src/local_config.c
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/ship_damage.c src/local_config.c -o $@

build/libdeath_event.so: src/death_event.c src/death_event.h src/notices.c
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/death_event.c src/notices.c -o $@

build/libprojectile_create.so: src/projectile_create.c src/projectile_create.h src/projectile_update.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/projectile_create.c -o $@

build/libprojectile_draw.so: src/projectile_draw.c src/projectile_draw.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/projectile_draw.c -o $@

build/libprojectile_frame.so: src/projectile_frame.c src/projectile_frame.h src/projectile_update.c src/projectile_update.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/projectile_frame.c src/projectile_update.c -o $@

CONTROL_SOURCES = src/offline_frame.c src/frame_update.c src/frame_clock.c src/camera.c src/offline_team.c src/player_team.c src/offline_control.c src/ordinary_control.c src/local_control.c src/ship_flight.c src/flight.c src/flight_input.c src/flight_boost.c src/flight_math.c src/ship_effect.c src/weapon_control.c src/position_dispatch.c
COMBAT_SOURCES = $(CONTROL_SOURCES) src/offline_simulation.c src/offline_pickups.c src/pickup_collect.c src/pickup_lifecycle.c src/offline_generation.c src/weapon_input.c src/weapon_action.c src/weapon_create.c src/offline_combat.c src/projectile_frame.c src/projectile_update.c src/projectile_create.c src/projectile_explosion.c src/ship_damage.c src/death_event.c
build/liboffline_combat_test.so: $(COMBAT_SOURCES) $(SHIP_WORLD_SOURCES) $(wildcard src/*.h) tests/offline_combat_bridge.c
	mkdir -p build
	$(CC) $(CFLAGS) -Isrc -fPIC -shared $(COMBAT_SOURCES) $(SHIP_WORLD_SOURCES) tests/offline_combat_bridge.c -o $@

build/libweapon_create.so: src/weapon_create.c src/weapon_create.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/weapon_create.c -o $@

build/libweapon_action.so: src/weapon_action.c src/weapon_action.h src/notices.c src/local_config.c
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/weapon_action.c src/notices.c src/local_config.c -o $@

build/libweapon_input.so: src/weapon_input.c src/weapon_input.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/weapon_input.c -o $@

build/libposition_dispatch.so: src/position_dispatch.c src/position_dispatch.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/position_dispatch.c -o $@

build/libship_draw_order.so: src/ship_draw_order.c src/ship_draw_order.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/ship_draw_order.c -o $@

build/libactor_layers.so: src/actor_layers.c src/actor_layers.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/actor_layers.c -o $@

build/libstarfield.so: src/starfield.c src/starfield.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/starfield.c -o $@
native-test: build/libstarfield.so

build/libhud_status.so: src/hud_status.c src/hud_status.h src/font.c src/font.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/hud_status.c src/font.c -o $@
native-test: build/libhud_status.so

build/libsurface_fill.so: src/surface_fill.c src/surface_fill.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/surface_fill.c -o $@
native-test: build/libsurface_fill.so

build/libhud_panel.so: src/hud_panel.c src/hud_panel.h src/font.c src/font.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/hud_panel.c src/font.c -o $@
native-test: build/libhud_panel.so

build/libscene_frame.so: src/scene_frame.c src/scene_frame.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/scene_frame.c -o $@
native-test: build/libscene_frame.so

build/libdisplay_recovery.so: src/display_recovery.c src/display_recovery.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/display_recovery.c -o $@
native-test: build/libdisplay_recovery.so

build/liblocal_control.so: src/local_control.c src/local_control.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/local_control.c -o $@
native-test: build/liblocal_control.so

build/libweapon_control.so: src/weapon_control.c src/weapon_control.h src/weapon_action.c src/weapon_input.c src/position_dispatch.c src/notices.c src/local_config.c
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/weapon_control.c src/weapon_action.c src/weapon_input.c src/position_dispatch.c src/notices.c src/local_config.c -o $@
native-test: build/libweapon_control.so

build/libship_flight.so: src/ship_flight.c src/ship_flight.h src/ship_effect.c src/ship_effect.h src/flight.c src/flight.h src/flight_input.c src/flight_input.h src/flight_boost.c src/flight_boost.h src/flight_math.c src/flight_math.h src/ship_create.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/ship_flight.c src/ship_effect.c src/flight.c src/flight_input.c src/flight_boost.c src/flight_math.c -o $@

build/libordinary_control.so: src/ordinary_control.c src/ordinary_control.h src/local_control.c src/ship_flight.c src/flight.c src/flight_input.c src/flight_boost.c src/flight_math.c src/ship_effect.c src/weapon_control.c src/weapon_action.c src/weapon_input.c src/position_dispatch.c src/notices.c src/local_config.c $(wildcard src/*.h)
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared $(filter %.c,$^) -o $@
native-test: build/libordinary_control.so

build/libframe_update.so: src/frame_update.c src/frame_update.h src/frame_clock.c src/frame_clock.h src/camera.c src/camera.h src/ship_spawn.c src/ship_reset.c
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared $(filter %.c,$^) -o $@
native-test: build/libframe_update.so

build/libplayer_team.so: src/player_team.c src/player_team.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/player_team.c -o $@
native-test: build/libplayer_team.so

build/liboffline_team_test.so: src/offline_team.c src/player_team.c $(SHIP_WORLD_SOURCES) $(wildcard src/*.h) tests/offline_team_bridge.c
	mkdir -p build
	$(CC) $(CFLAGS) -Isrc -fPIC -shared $(filter %.c,$^) -o $@
native-test: build/liboffline_team_test.so

build/libplayer_registry.so: src/player_registry.c src/player_registry.h src/player_team.c src/ship_create.c src/ship_reset.c src/notices.c
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared $(filter %.c,$^) -o $@
native-test: build/libplayer_registry.so

build/liboffline_init.so: src/offline_init.c src/offline_init.h src/player_registry.c src/player_team.c src/ship_create.c src/ship_reset.c src/notices.c src/local_config.c src/map.c src/level.c
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared $(filter %.c,$^) -o $@
native-test: build/liboffline_init.so

build/liboffline_start_test.so: src/offline_start.c src/player_team.c $(SHIP_WORLD_SOURCES) $(wildcard src/*.h) tests/offline_start_bridge.c
	mkdir -p build
	$(CC) $(CFLAGS) -Isrc -fPIC -shared $(filter %.c,$^) -o $@
native-test: build/liboffline_start_test.so

SESSION_SOURCES = $(sort src/display_input.c src/voice_input.c src/voice_recordings.c $(wildcard src/network_*.c) src/online_session.c src/online_gameplay.c src/inactive_position.c src/editor_input.c src/editor_records.c src/audio_logic.c $(COMBAT_SOURCES) $(SHIP_WORLD_SOURCES) src/offline_session.c src/spectator_control.c src/chat_input.c src/chat_dispatch.c src/chat_history.c src/chat_tips.c src/offline_team.c src/player_team.c src/list_mouse.c src/statistics_panel.c src/help_panel.c src/list_navigation.c src/attachment_command.c src/manual_warp.c src/capability_input.c src/exit_request.c src/offline_init.c src/offline_start.c src/player_registry.c src/ship_create.c src/starfield.c src/map.c src/level.c)
build/liboffline_session.so: $(SESSION_SOURCES) $(wildcard src/*.h)
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared $(SESSION_SOURCES) -o $@

build/offline-session-test: $(SESSION_SOURCES) tests/offline_session_test.c $(wildcard src/*.h)
	mkdir -p build
	$(CC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc $(SESSION_SOURCES) tests/offline_session_test.c -o $@

build/libexit_request.so: src/exit_request.c src/notices.c src/exit_request.h src/notices.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/exit_request.c src/notices.c -o $@

build/libcapability_input.so: src/capability_input.c src/capability_input.h src/ship_create.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/capability_input.c -o $@

build/libmanual_warp_test.so: src/manual_warp.c src/notices.c src/ship_spawn.c src/ship_reset.c src/local_config.c tests/manual_warp_bridge.c $(wildcard src/*.h)
	mkdir -p build
	$(CC) $(CFLAGS) -Isrc -fPIC -shared src/manual_warp.c src/notices.c src/ship_spawn.c src/ship_reset.c src/local_config.c tests/manual_warp_bridge.c -o $@

build/libattachment_command_test.so: src/attachment_command.c src/attachment.c src/notices.c tests/attachment_command_bridge.c $(wildcard src/*.h)
	mkdir -p build
	$(CC) $(CFLAGS) -Isrc -fPIC -shared src/attachment_command.c src/attachment.c src/notices.c tests/attachment_command_bridge.c -o $@

build/liblist_navigation.so: src/list_navigation.c src/list_navigation.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/list_navigation.c -o $@
build/libtext_panel.so: src/text_panel.c src/text_panel.h src/font.c src/font.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/text_panel.c src/font.c -o $@
build/libhelp_panel.so: src/help_panel.c src/help_panel.h src/text_panel.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/help_panel.c -o $@

build/libstatistics_panel.so: src/statistics_panel.c src/statistics_panel.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/statistics_panel.c -o $@

build/liblist_mouse.so: src/list_mouse.c src/list_mouse.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/list_mouse.c -o $@
build/libradar_map.so: src/radar_map.c src/radar_map.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/radar_map.c -o $@
build/libradar_markers.so: src/radar_markers.c src/radar_markers.h src/radar_map.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/radar_markers.c -o $@

build/libchat_input.so: src/chat_input.c src/chat_input.h
	@mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/chat_input.c -o $@

build/libchat_render.so: src/chat_render.c src/chat_render.h
	@mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/chat_render.c -o $@

build/libchat_dispatch.so: src/chat_dispatch.c src/chat_dispatch.h src/notices.c
	@mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/chat_dispatch.c src/notices.c -o $@

build/libchat_history.so: src/chat_history.c src/chat_history.h
	@mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/chat_history.c -o $@

build/libspectator_control.so: src/spectator_control.c src/spectator_control.h
	@mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/spectator_control.c -o $@


build/audio-session-test: $(SESSION_SOURCES) tests/audio_session_test.c $(wildcard src/*.h)
	@mkdir -p build
	$(CC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc $(filter-out src/offline_session.c,$(SESSION_SOURCES)) tests/audio_session_test.c -o $@

build/libeditor_render.so: src/editor_render.c src/editor_render.h
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -shared src/editor_render.c -o $@
build/editor-session-test: $(SESSION_SOURCES) tests/editor_session_test.c $(wildcard src/*.h)
	mkdir -p build
	$(CC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc $(SESSION_SOURCES) tests/editor_session_test.c -o $@

build/online-session-test: $(SESSION_SOURCES) tests/online_session_test.c $(wildcard src/*.h)
	$(CC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc $(SESSION_SOURCES) tests/online_session_test.c -o $@

.PHONY: voice-session-test
voice-session-test: build/voice-session-test
	UBSAN_OPTIONS=halt_on_error=1 ./build/voice-session-test
build/voice-session-test: $(SESSION_SOURCES) tests/voice_session_test.c $(wildcard src/*.h)
	$(CC) $(CFLAGS) -g -fsanitize=address,undefined -Isrc -o $@ tests/voice_session_test.c $(filter-out src/offline_session.c,$(SESSION_SOURCES)) -lm

build/libdisplay_input.so: src/display_input.c src/display_input.h
	mkdir -p build
	$(CC) $(CFLAGS) -shared -fPIC src/display_input.c -o $@
native-test: build/libdisplay_input.so

build/libvoice_input.so: src/voice_input.c src/voice_input.h src/notices.c src/notices.h
	mkdir -p build
	$(CC) $(CFLAGS) -shared -fPIC src/voice_input.c src/notices.c -o $@
build/libvoice_recordings.so: src/voice_recordings.c src/voice_recordings.h src/network_compression.c src/network_compression.h
	mkdir -p build
	$(CC) $(CFLAGS) -shared -fPIC src/voice_recordings.c src/network_compression.c -o $@
native-test: build/libvoice_input.so build/libvoice_recordings.so

build/libstartup_random.so: platform/startup_random.c platform/startup_random.h
	mkdir -p build
	$(CC) $(CFLAGS) -shared -fPIC platform/startup_random.c -o $@
native-test: build/libstartup_random.so
