$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$gradleText = [IO.File]::ReadAllText((Join-Path $repoRoot 'app\build.gradle'))
$ndkMatch = [regex]::Match($gradleText, 'ndkVersion\s+[''"]([^''"]+)[''"]')
if (-not $ndkMatch.Success) { throw 'Could not read ndkVersion from app/build.gradle.' }
$ndkRoot = $env:ANDROID_NDK_HOME
if (-not $ndkRoot) {
    $sdkRoot = $env:ANDROID_SDK_ROOT
    if (-not $sdkRoot) { $sdkRoot = $env:ANDROID_HOME }
    if (-not $sdkRoot) { throw 'Set ANDROID_SDK_ROOT or ANDROID_HOME to locate the Android NDK.' }
    $ndkRoot = Join-Path (Join-Path $sdkRoot 'ndk') $ndkMatch.Groups[1].Value
}
$clang = Join-Path $ndkRoot 'toolchains\llvm\prebuilt\windows-x86_64\bin\clang.exe'
if (-not (Test-Path -LiteralPath $clang)) { throw "NDK clang not found: $clang" }
$node = Get-Command node -ErrorAction Stop
$buildDir = Join-Path $repoRoot 'build\runtime-compatibility'
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
$wasm = Join-Path $buildDir 'tick_exit.wasm'
$windowWasm = Join-Path $buildDir 'window_label.wasm'
$reloadWasm = Join-Path $buildDir 'world_reload.wasm'
$soundWasm = Join-Path $buildDir 'sound_parse.wasm'
$soundTickWasm = Join-Path $buildDir 'sound_tick.wasm'
$gameClockWasm = Join-Path $buildDir 'game_clock.wasm'
$soundPcmWasm = Join-Path $buildDir 'sound_pcm.wasm'
$oopWasm = Join-Path $buildDir 'oop_label.wasm'
$rngWasm = Join-Path $buildDir 'rng.wasm'
$playerInputWasm = Join-Path $buildDir 'player_input.wasm'
$playerGlyphWasm = Join-Path $buildDir 'player_glyph.wasm'
$windowFileHintsWasm = Join-Path $buildDir 'window_file_hints.wasm'
$monitorWasm = Join-Path $buildDir 'monitor.wasm'
$spinningGunWasm = Join-Path $buildDir 'spinning_gun.wasm'
$blinkWallWasm = Join-Path $buildDir 'blink_wall.wasm'
$directionSeekWasm = Join-Path $buildDir 'direction_seek.wasm'
$boardShootWasm = Join-Path $buildDir 'board_shoot.wasm'
$oopMovementWasm = Join-Path $buildDir 'oop_movement.wasm'
$integratedGameplayWasm = Join-Path $buildDir 'integrated_gameplay.wasm'
$boardEdgeWasm = Join-Path $buildDir 'board_edge.wasm'
$boardEdgeFunction = Join-Path $buildDir 'board_edge_production.inc'
$editorFloodWasm = Join-Path $buildDir 'editor_flood_fill.wasm'
$editorFloodFunction = Join-Path $buildDir 'editor_flood_fill_production.inc'
$editorPlacementFunction = Join-Path $buildDir 'editor_placement_production.inc'
$statAddFunction = Join-Path $buildDir 'zoo_stat_add_production.inc'
$transporterWasm = Join-Path $buildDir 'transporter.wasm'
$transporterMoveFunction = Join-Path $buildDir 'transporter_move_production.inc'
$transporterTouchFunction = Join-Path $buildDir 'transporter_touch_production.inc'
$oopTokenC = Join-Path $buildDir 'libzoo\zoo_oop_token.c'
$statMoveFunction = Join-Path $buildDir 'zoo_stat_move_production.inc'
$source = Join-Path $PSScriptRoot 'tick_exit.c'
$windowSource = Join-Path $PSScriptRoot 'window_label.c'
$reloadSource = Join-Path $PSScriptRoot 'world_reload.c'
$zooGameSource = Join-Path $repoRoot 'third_party\libzoo\src\libzoo\zoo_game.c'
$soundSource = Join-Path $PSScriptRoot 'sound_parse.c'
$soundTickSource = Join-Path $PSScriptRoot 'sound_tick.c'
$gameClockSource = Join-Path $PSScriptRoot 'game_clock.c'
$soundPcmSource = Join-Path $PSScriptRoot 'sound_pcm.c'
$oopSource = Join-Path $PSScriptRoot 'oop_label.c'
$rngSource = Join-Path $PSScriptRoot 'rng.c'
$playerInputSource = Join-Path $PSScriptRoot 'player_input.c'
$playerGlyphSource = Join-Path $PSScriptRoot 'player_glyph.c'
$windowFileHintsSource = Join-Path $PSScriptRoot 'window_file_hints.c'
$monitorSource = Join-Path $PSScriptRoot 'monitor.c'
$spinningGunSource = Join-Path $PSScriptRoot 'spinning_gun.c'
$blinkWallSource = Join-Path $PSScriptRoot 'blink_wall.c'
$directionSeekSource = Join-Path $PSScriptRoot 'direction_seek.c'
$boardShootSource = Join-Path $PSScriptRoot 'board_shoot.c'
$oopMovementSource = Join-Path $PSScriptRoot 'oop_movement.c'
$boardEdgeSource = Join-Path $PSScriptRoot 'board_edge.c'
$boardEdgeExtractor = Join-Path $PSScriptRoot 'extract_board_edge.py'
$editorFloodExtractor = Join-Path $PSScriptRoot 'extract_editor_flood_fill.py'
$editorPlacementExtractor = Join-Path $PSScriptRoot 'extract_editor_placement.py'
$statAddExtractor = Join-Path $PSScriptRoot 'extract_stat_add.py'
$transporterExtractor = Join-Path $PSScriptRoot 'extract_transporter.py'
$clangArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-DZOO_CONFIG_USE_DOUBLE_FOR_MS', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-I', (Join-Path $PSScriptRoot 'include'),
    '-I', (Join-Path $repoRoot 'tools\serializer-compat\include'),
    '-I', (Join-Path $repoRoot 'app\src\main\cpp'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src'),
    '-Wl,--no-entry', '-Wl,--export=run_exit_tick_checks', '-Wl,--export=run_zero_speed_schedule_checks',
    '-Wl,--export=zero_speed_schedule_output_ptr', '-Wl,--export=run_game_schedule_checks',
    '-Wl,--export=game_schedule_output_ptr', '-Wl,--export=run_frame_scheduler_checks',
    '-Wl,--export=frame_scheduler_output_ptr', '-Wl,--export=run_frame_gameplay_checks',
    '-Wl,--export=frame_gameplay_output_ptr', '-Wl,--export=run_game_start_enter_checks', '-Wl,--export-memory', '-Wl,--gc-sections',
    '-Wl,--initial-memory=16777216', '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576',
    $source, '-o', $wasm
)
& $clang @clangArgs
if ($LASTEXITCODE -ne 0) { throw "WASM game tick harness compilation failed with exit code $LASTEXITCODE." }
$windowArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-I', (Join-Path $repoRoot 'tools\serializer-compat\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src'),
    '-Wl,--no-entry', '-Wl,--export=run_window_label_checks', '-Wl,--export-memory', '-Wl,--gc-sections',
    '-Wl,--initial-memory=16777216', '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576',
    $windowSource, '-o', $windowWasm
)
& $clang @windowArgs
if ($LASTEXITCODE -ne 0) { throw "WASM window-label harness compilation failed with exit code $LASTEXITCODE." }
$reloadArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-I', (Join-Path $PSScriptRoot 'include'),
    '-I', (Join-Path $repoRoot 'tools\serializer-compat\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src'),
    '-Wl,--no-entry', '-Wl,--export=run_world_reload_checks',
    '-Wl,--export=run_world_play_missing_reload_capture', '-Wl,--export=world_reload_missing_output_ptr',
    '-Wl,--export=world_reload_missing_output_count', '-Wl,--export-memory', '-Wl,--gc-sections',
    '-Wl,--allow-undefined',
    '-Wl,--initial-memory=16777216', '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576',
    $reloadSource, $zooGameSource, '-o', $reloadWasm
)
& $clang @reloadArgs
if ($LASTEXITCODE -ne 0) { throw "WASM world-reload harness compilation failed with exit code $LASTEXITCODE." }
$soundArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-I', (Join-Path $repoRoot 'tools\serializer-compat\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src'),
    '-Wl,--no-entry', '-Wl,--export=run_sound_parse_checks', '-Wl,--export=sound_input_ptr',
    '-Wl,--export=sound_output_ptr', '-Wl,--export=run_sound_parse_from_input', '-Wl,--export-memory', '-Wl,--gc-sections',
    '-Wl,--initial-memory=16777216', '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576',
    $soundSource, '-o', $soundWasm
)
& $clang @soundArgs
if ($LASTEXITCODE -ne 0) { throw "WASM sound parser harness compilation failed with exit code $LASTEXITCODE." }
$soundTickArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-I', (Join-Path $repoRoot 'tools\serializer-compat\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src'),
    '-Wl,--no-entry', '-Wl,--export=run_sound_tick_checks', '-Wl,--export=run_sound_pitch_table_checks',
    '-Wl,--export=sound_pitch_output_ptr', '-Wl,--export-memory', '-Wl,--gc-sections',
    '-Wl,--initial-memory=16777216', '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576',
    $soundTickSource, '-o', $soundTickWasm
)
& $clang @soundTickArgs
if ($LASTEXITCODE -ne 0) { throw "WASM sound-tick compatibility harness compilation failed with exit code $LASTEXITCODE." }
$soundPcmArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-I', (Join-Path $repoRoot 'tools\serializer-compat\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\include'),
    '-I', (Join-Path $repoRoot 'app\src\main\cpp'),
    '-Wl,--no-entry', '-Wl,--export=run_sound_pcm_checks', '-Wl,--export-memory', '-Wl,--gc-sections',
    '-Wl,--initial-memory=16777216', '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576',
    $soundPcmSource, (Join-Path $repoRoot 'app\src\main\cpp\zzt_sound_pcm.c'), '-o', $soundPcmWasm
)
& $clang @soundPcmArgs
if ($LASTEXITCODE -ne 0) { throw "WASM sound PCM harness compilation failed with exit code $LASTEXITCODE." }
$oopTokenDir = Split-Path -Parent $oopTokenC
New-Item -ItemType Directory -Force -Path $oopTokenDir | Out-Null
$python = Get-Command python -ErrorAction Stop
& $python.Source (Join-Path $repoRoot 'third_party\libzoo\tools\tok2c.py') `
    (Join-Path $repoRoot 'third_party\libzoo\src\libzoo\zoo_oop_token.tok') $oopTokenC
if ($LASTEXITCODE -ne 0) { throw "OOP token generation failed with exit code $LASTEXITCODE." }
& $python.Source (Join-Path $PSScriptRoot 'extract_stat_move.py') `
    (Join-Path $repoRoot 'third_party\libzoo\src\libzoo\zoo_game.c') $statMoveFunction
if ($LASTEXITCODE -ne 0) { throw "Production stat-move extraction failed with exit code $LASTEXITCODE." }
$oopArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-I', (Join-Path $repoRoot 'tools\serializer-compat\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src'),
    '-I', $buildDir,
    '-Wl,--no-entry', '-Wl,--export=run_oop_label_checks', '-Wl,--export=run_oop_flag_checks',
    '-Wl,--export=run_oop_window_title_checks', '-Wl,--export=run_oop_try_command_checks',
	'-Wl,--export=run_oop_if_condition_checks', '-Wl,--export=oop_if_output_ptr',
	'-Wl,--export=run_oop_any_element_check', '-Wl,--export=oop_any_name_ptr',
	'-Wl,--export=oop_any_output_ptr',
	'-Wl,--export=run_oop_counter_overflow_checks',
	'-Wl,--export=oop_counter_output_ptr',
	'-Wl,--export=run_oop_zap_restore_checks',
	'-Wl,--export=run_oop_bind_self_checks',
	'-Wl,--export=run_oop_bind_selector_checks', '-Wl,--export=oop_bind_selector_output_ptr',
	'-Wl,--export=run_oop_flow_control_checks',
	'-Wl,--export=run_oop_flow_capture_checks', '-Wl,--export=oop_flow_output_ptr',
	'-Wl,--export=run_oop_random_walk_capture_checks', '-Wl,--export=oop_random_walk_output_ptr',
	'-Wl,--export=run_oop_object_command_checks',
	'-Wl,--export=run_oop_char_capture_checks', '-Wl,--export=oop_char_output_ptr',
	'-Wl,--export=run_oop_put_capture_checks', '-Wl,--export=oop_put_output_ptr',
	'-Wl,--export=run_oop_change_capture_checks', '-Wl,--export=oop_change_output_ptr',
	'-Wl,--export=run_oop_flag_and_send_command_checks',
	'-Wl,--export=run_oop_send_capture_checks', '-Wl,--export=oop_send_capture_output_ptr',
	'-Wl,--export=run_oop_flag_state_capture_checks', '-Wl,--export=oop_flag_capture_output_ptr',
	'-Wl,--export=run_oop_combat_and_play_checks',
	'-Wl,--export=run_oop_direction_parser_checks', '-Wl,--export=run_oop_tile_parser_checks',
	'-Wl,--export=run_oop_place_tile_checks', '-Wl,--export=run_oop_line_reader_checks', '-Wl,--allow-undefined',
    '-Wl,--export=run_oop_value_from_input',
    '-Wl,--export=run_oop_word_from_input', '-Wl,--export=oop_word_result_ptr',
    '-Wl,--export=oop_value_input_ptr', '-Wl,--export=oop_value_result_position',
    '-Wl,--export=run_oop_line_from_input', '-Wl,--export=oop_line_input_ptr',
    '-Wl,--export=oop_line_output_ptr', '-Wl,--export=oop_line_result_position',
    '-Wl,--export-memory', '-Wl,--gc-sections',
    '-Wl,--initial-memory=16777216', '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576',
    $oopSource, '-o', $oopWasm
)
& $clang @oopArgs
if ($LASTEXITCODE -ne 0) { throw "WASM OOP-label harness compilation failed with exit code $LASTEXITCODE." }
$rngArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src\libzoo'),
    '-Wl,--no-entry', '-Wl,--export=run_rng_checks',
    '-Wl,--export=rng_vector_set_seed', '-Wl,--export=rng_vector_next',
    '-Wl,--export=rng_vector_seed_low', '-Wl,--export=rng_vector_seed_high',
    '-Wl,--export-memory', '-Wl,--gc-sections',
    '-Wl,--initial-memory=16777216', '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576',
    $rngSource, '-o', $rngWasm
)
& $clang @rngArgs
if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal RNG harness compilation failed with exit code $LASTEXITCODE." }
$playerInputArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-I', (Join-Path $repoRoot 'tools\serializer-compat\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\include'),
	'-I', (Join-Path $repoRoot 'third_party\libzoo\src\ui'),
	'-Wl,--no-entry', '-Wl,--export=run_player_input_checks', '-Wl,--export=player_input_arrow_burst_output_ptr', '-Wl,--export=player_input_key_burst_output_ptr',
	'-Wl,--export=player_input_wrapped_burst_output_ptr', '-Wl,--export=run_input_length_matrix_checks',
	'-Wl,--export=player_input_length_matrix_output_ptr', '-Wl,--export-memory', '-Wl,--gc-sections',
    '-Wl,--initial-memory=16777216', '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576',
    $playerInputSource, `
    (Join-Path $repoRoot 'third_party\libzoo\src\libzoo\zoo_input.c'), `
    (Join-Path $repoRoot 'third_party\libzoo\src\ui\zoo_ui_input.c'), '-o', $playerInputWasm
)
& $clang @playerInputArgs
if ($LASTEXITCODE -ne 0) { throw "Player input compatibility harness compilation failed with exit code $LASTEXITCODE." }
$playerGlyphArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-I', (Join-Path $repoRoot 'tools\serializer-compat\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src\libzoo'),
    '-Wl,--no-entry', '-Wl,--export=run_player_glyph_checks', '-Wl,--export-memory', '-Wl,--gc-sections',
    '-Wl,--initial-memory=16777216', '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576',
    $playerGlyphSource, '-o', $playerGlyphWasm
)
& $clang @playerGlyphArgs
if ($LASTEXITCODE -ne 0) { throw "Player glyph compatibility harness compilation failed with exit code $LASTEXITCODE." }
$spinningGunArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-I', (Join-Path $repoRoot 'tools\serializer-compat\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src\libzoo'),
    '-Wl,--no-entry', '-Wl,--export=run_spinning_gun_checks', '-Wl,--export=get_spinning_gun_trace_count',
    '-Wl,--export=get_spinning_gun_trace', '-Wl,--export-memory', '-Wl,--gc-sections',
    '-Wl,--initial-memory=16777216', '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576',
    $spinningGunSource, '-o', $spinningGunWasm
)
& $clang @spinningGunArgs
if ($LASTEXITCODE -ne 0) { throw "Spinning-gun compatibility harness compilation failed with exit code $LASTEXITCODE." }
$blinkWallArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-I', (Join-Path $repoRoot 'tools\serializer-compat\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src\libzoo'),
    '-Wl,--no-entry', '-Wl,--export=run_blink_wall_compat_checks', '-Wl,--export-memory', '-Wl,--gc-sections',
    '-Wl,--initial-memory=16777216', '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576',
    $blinkWallSource, '-o', $blinkWallWasm
)
& $clang @blinkWallArgs
if ($LASTEXITCODE -ne 0) { throw "Blink-wall compatibility harness compilation failed with exit code $LASTEXITCODE." }
$directionSeekArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-I', (Join-Path $repoRoot 'tools\serializer-compat\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src\libzoo'),
    '-Wl,--no-entry', '-Wl,--export=run_direction_seek_checks', '-Wl,--export-memory', '-Wl,--gc-sections',
    '-Wl,--initial-memory=16777216', '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576',
    $directionSeekSource, '-o', $directionSeekWasm
)
& $clang @directionSeekArgs
if ($LASTEXITCODE -ne 0) { throw "Direction-seeking compatibility harness compilation failed with exit code $LASTEXITCODE." }
$boardShootArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-I', (Join-Path $repoRoot 'tools\serializer-compat\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src\libzoo'),
    '-Wl,--no-entry', '-Wl,--export=run_board_shoot_checks', '-Wl,--export-memory', '-Wl,--gc-sections', '-Wl,--allow-undefined',
    '-Wl,--initial-memory=16777216', '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576',
    $boardShootSource, '-o', $boardShootWasm
)
& $clang @boardShootArgs
if ($LASTEXITCODE -ne 0) { throw "Board-shoot compatibility harness compilation failed with exit code $LASTEXITCODE." }
$oopMovementArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-I', (Join-Path $repoRoot 'tools\serializer-compat\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src\libzoo'),
    '-I', $buildDir,
    '-Wl,--no-entry', '-Wl,--export=run_oop_production_movement_checks',
    '-Wl,--export=run_player_tick_capture_checks',
    '-Wl,--export=player_movement_output_ptr', '-Wl,--export=player_shoot_output_ptr',
    '-Wl,--export=player_key_output_ptr', '-Wl,--export=player_death_output_ptr',
    '-Wl,--export=player_paused_move_output_ptr',
    '-Wl,--export=run_oop_bind_reuse_addstat_checks',
	'-Wl,--export=run_oop_bind_text_window_reuse_checks',
    '-Wl,--export=run_slime_tick_capture_checks', '-Wl,--export=slime_tick_output_ptr',
    '-Wl,--export=run_bomb_tick_capture_checks', '-Wl,--export=bomb_tick_output_ptr',
	'-Wl,--export=run_conveyor_tick_capture_checks', '-Wl,--export=conveyor_tick_output_ptr',
	'-Wl,--export=run_star_tick_capture_checks', '-Wl,--export=star_tick_output_ptr',
	'-Wl,--export=run_tiger_tick_capture_checks', '-Wl,--export=tiger_tick_output_ptr',
	'-Wl,--export=run_centipede_tick_capture_checks', '-Wl,--export=centipede_tick_output_ptr',
	'-Wl,--export=run_element_draw_capture_checks', '-Wl,--export=element_draw_output_ptr',
	'-Wl,--export=run_oop_object_tick_capture_checks', '-Wl,--export=object_tick_output_ptr',
	'-Wl,--export=run_oop_object_touch_capture_checks', '-Wl,--export=object_touch_output_ptr',
	'-Wl,--export=run_message_timer_capture_checks', '-Wl,--export=message_timer_output_ptr',
	'-Wl,--export=run_blink_wall_tick_capture_checks', '-Wl,--export=blink_wall_tick_output_ptr',
	'-Wl,--export=run_scroll_tick_capture_checks', '-Wl,--export=scroll_tick_output_ptr',
	'-Wl,--export=run_scroll_touch_capture_checks', '-Wl,--export=scroll_touch_output_ptr',
	'-Wl,--export=run_ruffian_tick_capture_checks', '-Wl,--export=ruffian_tick_output_ptr',
	'-Wl,--export=run_bear_tick_capture_checks', '-Wl,--export=bear_tick_output_ptr',
	'-Wl,--export=run_shark_tick_capture_checks', '-Wl,--export=shark_tick_output_ptr',
	'-Wl,--export=run_lion_tick_capture_checks', '-Wl,--export=lion_tick_output_ptr',
	'-Wl,--export=run_pusher_tick_capture_checks', '-Wl,--export=pusher_tick_output_ptr',
	'-Wl,--export=run_duplicator_tick_capture_checks', '-Wl,--export=duplicator_tick_output_ptr',
    '-Wl,--export-memory', '-Wl,--gc-sections', '-Wl,--allow-undefined',
    '-Wl,--initial-memory=16777216', '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576',
    $oopMovementSource, '-o', $oopMovementWasm
)
& $clang @oopMovementArgs
if ($LASTEXITCODE -ne 0) { throw "OOP production-movement compatibility harness compilation failed with exit code $LASTEXITCODE." }
$integratedGameplayArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections', '-DZZT_INTEGRATED_GAMEPLAY_ONLY',
    '-I', (Join-Path $repoRoot 'tools\serializer-compat\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src\libzoo'),
    '-I', (Join-Path $PSScriptRoot 'include'),
    '-I', $buildDir,
	'-Wl,--no-entry', '-Wl,--export=run_integrated_gameplay_checks',
	'-Wl,--export=run_integrated_title_lifecycle_checks',
	'-Wl,--export=run_integrated_title_refused_abort_checks',
	'-Wl,--export=run_integrated_title_saved_world_play_checks',
	'-Wl,--export=run_integrated_title_world_select_play_checks',
	'-Wl,--export=run_integrated_end_play_checks',
	'-Wl,--export=integrated_end_play_output_ptr', '-Wl,--export=integrated_end_play_output_rows',
	'-Wl,--export=run_integrated_end_play_loop_check',
	'-Wl,--export=integrated_end_play_loop_output_ptr', '-Wl,--export=integrated_end_play_loop_output_count',
	'-Wl,--export=run_integrated_refused_end_play_loop_checks',
	'-Wl,--export=integrated_refused_end_play_loop_output_ptr', '-Wl,--export=integrated_refused_end_play_loop_output_count',
	'-Wl,--export=run_integrated_title_quit_checks',
	'-Wl,--export=integrated_title_quit_output_ptr', '-Wl,--export=integrated_title_quit_output_rows',
	'-Wl,--export=run_integrated_title_restore_checks',
	'-Wl,--export=run_integrated_title_restore_retry_checks',
	'-Wl,--export=run_integrated_title_restore_failure_checks',
	'-Wl,--export=integrated_title_restore_failure_fields_ptr',
	'-Wl,--export=integrated_title_restore_failure_field_count',
	'-Wl,--export=integrated_title_restore_failure_error',
	'-Wl,--export=integrated_demo_world_bytes_ptr', '-Wl,--export=integrated_demo_world_bytes_capacity',
	'-Wl,--export=run_integrated_demo_gameplay_checks',
	'-Wl,--export=run_integrated_wall_pusher_duplicator_checks',
	'-Wl,--export=run_integrated_combat_key_pickup_checks',
	'-Wl,--export=run_integrated_script_stat_order_checks',
	'-Wl,--export=run_integrated_board_neighbor_passage_checks',
	'-Wl,--export=run_integrated_board_edge_actual_transition_checks',
	'-Wl,--export=integrated_board_edge_actual_output_ptr',
	'-Wl,--export=integrated_board_edge_actual_output_rows',
	'-Wl,--export=integrated_board_edge_actual_output_columns',
	'-Wl,--export=run_transition_table_capture_checks', '-Wl,--export=transition_table_output_ptr',
	'-Wl,--export=run_integrated_dark_room_torch_energizer_timers_death_checks',
	'-Wl,--export=run_integrated_pause_scheduler_checks',
	'-Wl,--export=run_integrated_save_restore_world_reload_checks',
	'-Wl,--export=run_integrated_editor_brd_roundtrip_checks',
	'-Wl,--export=integrated_editor_brd_bytes_ptr', '-Wl,--export=integrated_editor_brd_bytes_length',
	'-Wl,--export=integrated_f10_bad_world_bytes_ptr', '-Wl,--export=integrated_f10_bad_world_bytes_capacity',
	'-Wl,--export=run_integrated_f10_checks', '-Wl,--export=integrated_f10_bad_world_load_result',
	'-Wl,--export=integrated_f10_high_score_save_bytes_ptr', '-Wl,--export=integrated_f10_high_score_save_length',
    '-Wl,--export=integrated_gameplay_output_ptr', '-Wl,--export=integrated_gameplay_trace_ptr',
    '-Wl,--export=integrated_gameplay_snapshots_ptr', '-Wl,--export=integrated_gameplay_checkpoint_headers_ptr',
    '-Wl,--export=integrated_gameplay_snapshot_count', '-Wl,--export=integrated_gameplay_snapshot_lengths_ptr',
    '-Wl,--export=integrated_gameplay_snapshot_stride', '-Wl,--export=integrated_gameplay_output_length',
    '-Wl,--export=integrated_gameplay_sound_events_ptr', '-Wl,--export=integrated_gameplay_sound_event_count',
    '-Wl,--export=integrated_gameplay_sound_event_overflow', '-Wl,--export=integrated_gameplay_sound_event_stride',
	'-Wl,--export=integrated_gameplay_cell_events_ptr', '-Wl,--export=integrated_gameplay_cell_event_count',
	'-Wl,--export=integrated_gameplay_cell_event_overflow', '-Wl,--export=integrated_gameplay_cell_event_stride',
	'-Wl,--export=integrated_gameplay_cell_context_events_ptr', '-Wl,--export=integrated_gameplay_cell_context_stride',
	'-Wl,--export=integrated_gameplay_cell_idempotent_write_count',
	'-Wl,--export=integrated_gameplay_save_bytes_ptr', '-Wl,--export=integrated_gameplay_save_length',
	'-Wl,--export=integrated_gameplay_save_result', '-Wl,--export=integrated_gameplay_save_overflow',
	'-Wl,--export=integrated_gameplay_reload_input_bytes_ptr', '-Wl,--export=integrated_gameplay_reload_input_capacity',
	'-Wl,--export=integrated_gameplay_reload_from_pascal_save', '-Wl,--export=integrated_gameplay_reload_result',
	'-Wl,--export=integrated_gameplay_mutate_for_f08_restore',
	'-Wl,--export=integrated_gameplay_reload_f08_save_from_pascal',
	'-Wl,--export=integrated_gameplay_reload_f08_world_from_pascal',
	'-Wl,--export=integrated_gameplay_finish_roundtrip', '-Wl,--export=integrated_gameplay_roundtrip_save_bytes_ptr',
	'-Wl,--export=integrated_gameplay_roundtrip_save_length', '-Wl,--export=integrated_gameplay_roundtrip_save_result',
	'-Wl,--export=integrated_gameplay_roundtrip_save_overflow',
    '-Wl,--export-memory', '-Wl,--gc-sections', '-Wl,--allow-undefined',
    '-Wl,--initial-memory=16777216', '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576',
    $oopMovementSource, '-o', $integratedGameplayWasm
)
& $clang @integratedGameplayArgs
if ($LASTEXITCODE -ne 0) { throw "Integrated gameplay harness compilation failed with exit code $LASTEXITCODE." }
$boardEdgeArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-I', (Join-Path $repoRoot 'tools\serializer-compat\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src\libzoo'),
    '-I', $buildDir,
    '-Wl,--no-entry', '-Wl,--export=run_board_edge_compat_checks',
    '-Wl,--export=board_edge_output_ptr', '-Wl,--export=board_edge_output_rows',
    '-Wl,--export=board_edge_output_columns', '-Wl,--export-memory', '-Wl,--gc-sections',
    '-Wl,--initial-memory=16777216', '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576',
    $boardEdgeSource, '-o', $boardEdgeWasm
)
& $python.Source $boardEdgeExtractor `
    (Join-Path $repoRoot 'third_party\libzoo\src\libzoo\zoo_elements.c') $boardEdgeFunction
if ($LASTEXITCODE -ne 0) { throw "Board-edge production function extraction failed with exit code $LASTEXITCODE." }
& $clang @boardEdgeArgs
if ($LASTEXITCODE -ne 0) { throw "Board-edge compatibility harness compilation failed with exit code $LASTEXITCODE." }
& $python.Source $transporterExtractor `
    (Join-Path $repoRoot 'third_party\libzoo\src\libzoo\zoo_elements.c') `
    $transporterMoveFunction $transporterTouchFunction
if ($LASTEXITCODE -ne 0) { throw "Transporter production function extraction failed with exit code $LASTEXITCODE." }
$transporterArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-I', (Join-Path $repoRoot 'tools\serializer-compat\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src\libzoo'),
    '-I', $buildDir,
    '-Wl,--no-entry', '-Wl,--export=run_transporter_compat_checks', '-Wl,--export-memory', '-Wl,--gc-sections',
    '-Wl,--initial-memory=16777216', '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576',
    (Join-Path $PSScriptRoot 'transporter.c'), '-o', $transporterWasm
)
& $clang @transporterArgs
if ($LASTEXITCODE -ne 0) { throw "Transporter compatibility harness compilation failed with exit code $LASTEXITCODE." }
$windowFileHintsArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-I', (Join-Path $repoRoot 'tools\serializer-compat\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src\libzoo'),
    '-Wl,--no-entry', '-Wl,--export=run_window_file_hint_checks', '-Wl,--export-memory', '-Wl,--gc-sections',
    '-Wl,--initial-memory=16777216', '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576',
    $windowFileHintsSource, '-o', $windowFileHintsWasm
)
& $clang @windowFileHintsArgs
if ($LASTEXITCODE -ne 0) { throw "Window file-hint compatibility harness compilation failed with exit code $LASTEXITCODE." }
$monitorArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-I', (Join-Path $repoRoot 'tools\serializer-compat\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src\libzoo'),
    '-Wl,--no-entry', '-Wl,--export=run_monitor_compat_checks', '-Wl,--export-memory', '-Wl,--gc-sections',
    '-Wl,--initial-memory=16777216', '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576',
    $monitorSource, '-o', $monitorWasm
)
& $clang @monitorArgs
if ($LASTEXITCODE -ne 0) { throw "Monitor compatibility harness compilation failed with exit code $LASTEXITCODE." }
$gameClockArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-I', (Join-Path $PSScriptRoot 'include'),
    '-I', (Join-Path $repoRoot 'tools\serializer-compat\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src\libzoo'),
    '-Wl,--no-entry', '-Wl,--export=run_game_clock_checks', '-Wl,--export=game_clock_output_ptr',
    '-Wl,--export-memory', '-Wl,--gc-sections',
    '-Wl,--initial-memory=16777216', '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576',
    $gameClockSource, '-o', $gameClockWasm
)
& $clang @gameClockArgs
if ($LASTEXITCODE -ne 0) { throw "Production game-clock compatibility harness compilation failed with exit code $LASTEXITCODE." }
$editorFloodArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-DZOO_CONFIG_ENABLE_EDITOR_CONSTANTS', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-I', (Join-Path $repoRoot 'third_party\libzoo\include'),
    '-I', $buildDir,
    '-Wl,--no-entry', '-Wl,--export=run_editor_fill_self_checks',
    '-Wl,--export=run_editor_fill_case', '-Wl,--export=editor_fill_snapshot_ptr',
    '-Wl,--export=editor_fill_tile_element', '-Wl,--export=editor_fill_tile_color',
    '-Wl,--export=run_editor_placement_checks', '-Wl,--export=editor_placement_output_ptr',
    '-Wl,--export-memory', '-Wl,--gc-sections',
    '-Wl,--initial-memory=16777216', '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576',
    (Join-Path $PSScriptRoot 'editor_flood_fill.c'),
    (Join-Path $repoRoot 'third_party\libzoo\src\libzoo\zoo_editor.c'),
    '-o', $editorFloodWasm
)
& $python.Source $editorFloodExtractor `
    (Join-Path $repoRoot 'app\src\main\cpp\native-lib.c') $editorFloodFunction
if ($LASTEXITCODE -ne 0) { throw "Android editorFloodFill extraction failed with exit code $LASTEXITCODE." }
& $python.Source $editorPlacementExtractor `
    (Join-Path $repoRoot 'app\src\main\cpp\native-lib.c') $editorPlacementFunction
if ($LASTEXITCODE -ne 0) { throw "Android editor placement helper extraction failed with exit code $LASTEXITCODE." }
& $python.Source $statAddExtractor `
    (Join-Path $repoRoot 'third_party\libzoo\src\libzoo\zoo_game.c') $statAddFunction
if ($LASTEXITCODE -ne 0) { throw "Production stat-add extraction failed with exit code $LASTEXITCODE." }
& $clang @editorFloodArgs
if ($LASTEXITCODE -ne 0) { throw "Shared LibZoo editor flood-fill harness compilation failed with exit code $LASTEXITCODE." }
& $node.Source (Join-Path $PSScriptRoot 'run.js') $wasm $windowWasm $reloadWasm $soundWasm $oopWasm $rngWasm $playerInputWasm $spinningGunWasm $blinkWallWasm $directionSeekWasm $boardEdgeWasm $transporterWasm $soundPcmWasm $playerGlyphWasm $windowFileHintsWasm $monitorWasm $boardShootWasm $oopMovementWasm $soundTickWasm $gameClockWasm $integratedGameplayWasm
if ($LASTEXITCODE -ne 0) { throw "Game tick exit checks failed with exit code $LASTEXITCODE." }
& $node.Source (Join-Path $PSScriptRoot 'editor-flood-fill-smoke.js') $editorFloodWasm
if ($LASTEXITCODE -ne 0) { throw "Shared LibZoo editor flood-fill self-check failed with exit code $LASTEXITCODE." }
$javac = Get-Command javac -ErrorAction Stop
$java = Get-Command java -ErrorAction Stop
$videoAdapter = Join-Path $repoRoot 'app\src\main\java\com\kerry\zzt\VideoAttributes.java'
$videoHarness = Join-Path $PSScriptRoot 'VideoAttributesCompat.java'
& $javac.Source '-encoding' 'UTF-8' '-d' $buildDir $videoAdapter $videoHarness
if ($LASTEXITCODE -ne 0) { throw "Video-attribute compatibility harness compilation failed with exit code $LASTEXITCODE." }
& $java.Source '-cp' $buildDir 'com.kerry.zzt.VideoAttributesCompat'
if ($LASTEXITCODE -ne 0) { throw "Video-attribute compatibility checks failed with exit code $LASTEXITCODE." }
