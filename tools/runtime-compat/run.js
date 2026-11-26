const fs = require('node:fs');

function pascalSoundParse(input) {
  const tones = { A: 9, B: 11, C: 0, D: 2, E: 4, F: 5, G: 7 };
  const output = [];
  let octave = 3;
  let duration = 1;
  for (let i = 0; i < input.length; i++) {
    const ch = input[i].toUpperCase();
    if (ch === 'T') duration = 1;
    else if (ch === 'S') duration = 2;
    else if (ch === 'I') duration = 4;
    else if (ch === 'Q') duration = 8;
    else if (ch === 'H') duration = 16;
    else if (ch === 'W') duration = 32;
    else if (ch === '.') {
      const bits = ((duration * 3) % 65536 + 65536) % 65536;
      const product = bits < 32768 ? bits : bits - 65536;
      duration = Math.trunc(Math.trunc(product) / 2);
    } else if (ch === '3') duration = Math.trunc(duration / 3);
    else if (ch === '+') octave = Math.min(6, octave + 1);
    else if (ch === '-') octave = Math.max(1, octave - 1);
    else if (Object.prototype.hasOwnProperty.call(tones, ch)) {
      let tone = tones[ch];
      if (input[i + 1] === '!') { tone--; i++; }
      else if (input[i + 1] === '#') { tone++; i++; }
      output.push((octave * 16 + tone) & 0xff, duration & 0xff);
    } else if (ch === 'X') {
      output.push(0, duration & 0xff);
    } else if (ch >= '0' && ch <= '9') {
      output.push((ch.charCodeAt(0) + 0xf0 - '0'.charCodeAt(0)) & 0xff, duration & 0xff);
    }
  }
  return Buffer.from(output);
}

(async () => {
  const module = await WebAssembly.compile(fs.readFileSync(process.argv[2]));
  const instance = await WebAssembly.instantiate(module, {});
  const result = instance.exports.run_exit_tick_checks();
  if (result !== 0) throw new Error(`Exit tick check returned ${result}`);
  console.log('Game exit flag: stops subsequent stats immediately; normal ticks and deferred-stat completion still work');
  const zeroSpeedResult = instance.exports.run_zero_speed_schedule_checks();
  if (zeroSpeedResult !== 0) throw new Error(`Zero-speed cycle check returned ${zeroSpeedResult}`);
  const zeroSpeedOutput = new Int16Array(instance.exports.memory.buffer,
    instance.exports.zero_speed_schedule_output_ptr(), 5);
  const expectedZeroSpeedOutput = [5, 5, 0, 0, 0];
  for (let i = 0; i < expectedZeroSpeedOutput.length; i++) {
    if (zeroSpeedOutput[i] !== expectedZeroSpeedOutput[i]) {
      throw new Error(`Production zero-speed schedule field ${i + 1} differs: ${zeroSpeedOutput[i]} != ${expectedZeroSpeedOutput[i]}`);
    }
  }
  console.log('Zero-speed schedule: five game cycles advance immediately without PIT or tick-counter time');
  const gameScheduleResult = instance.exports.run_game_schedule_checks();
  if (gameScheduleResult !== 0) throw new Error(`Deterministic game schedule check returned ${gameScheduleResult}`);
  const gameSchedule = new Int16Array(instance.exports.memory.buffer,
    instance.exports.game_schedule_output_ptr(), 117);
  const expectedScheduleCases = [
    [0, 0, 0, 0], [0, 5980, 0, 0],
    [1, 0, 0, 0], [1, 5980, 0, 0],
    [6, 0, 0, 0], [6, 5980, 0, 0],
    [9, 0, 0, 0], [9, 5980, 0, 0], [6, 0, 2, 4],
  ];
  for (let caseIndex = 0; caseIndex < expectedScheduleCases.length; caseIndex++) {
    const row = gameSchedule.subarray(caseIndex * 13, caseIndex * 13 + 13);
    const [speed, startHsecs, stallAfterPoll, stallExtraPits] = expectedScheduleCases[caseIndex];
    if (row[0] !== speed || row[1] !== startHsecs || row[2] !== stallAfterPoll
        || row[3] !== stallExtraPits || row[4] !== 5 || row[5] !== 5
        || row[6] !== 0 || row[7] !== row[12]) {
      throw new Error(`Malformed deterministic schedule capture for case ${caseIndex + 1}: ${Array.from(row).join(' ')}`);
    }
    let previousHsecs = startHsecs;
    for (let cycle = 0; cycle < 5; cycle++) {
      const elapsedHsecs = (row[8 + cycle] - previousHsecs + 6000) % 6000;
      if (elapsedHsecs < speed * 2) {
        throw new Error(`Schedule case ${caseIndex + 1} advanced early at cycle ${cycle + 1}`);
      }
      previousHsecs = row[8 + cycle];
    }
  }
  console.log('Game schedule: speeds 0, 1, 6, and 9 at phases 0 and 5980, plus a four-PIT stall, match across five cycles');
  const frameScheduleResult = instance.exports.run_frame_scheduler_checks();
  if (frameScheduleResult !== 0) throw new Error(`Android frame scheduler check returned ${frameScheduleResult}`);
  const frameSchedule = new Int16Array(instance.exports.memory.buffer,
    instance.exports.frame_scheduler_output_ptr(), 11);
  const expectedFrameSchedule = [36, 1, 5, 0, 0, 2, 0, 0, 1, 1, 1];
  for (let field = 0; field < expectedFrameSchedule.length; field++) {
    if (frameSchedule[field] !== expectedFrameSchedule[field]) {
      throw new Error(`Android frame scheduler field ${field + 1} differs: ${frameSchedule[field]} != ${expectedFrameSchedule[field]}`);
    }
  }
  console.log('Android frame scheduler: 60 Hz fractional PIT accumulation, sub-second catch-up, >1 second rebase, backward timestamp, and game-deadline transitions match production helper');
  const frameGameplayResult = instance.exports.run_frame_gameplay_checks();
  if (frameGameplayResult !== 0) throw new Error(`Android frame gameplay check returned ${frameGameplayResult}`);
  const frameGameplay = new Int16Array(instance.exports.memory.buffer,
    instance.exports.frame_gameplay_output_ptr(), 24);
  const expectedFrameGameplay = [
    [0, 15, 5, 5, 0, 82, 16, 32, 49, 65, 82],
    [1, 18, 5, 5, 0, 98, 32, 49, 65, 82, 98],
  ];
  for (let caseIndex = 0; caseIndex < expectedFrameGameplay.length; caseIndex++) {
    const row = frameGameplay.subarray(caseIndex * 12, caseIndex * 12 + 12);
    const [stall, pits, cycles, delta, stat, counter, ...times] = expectedFrameGameplay[caseIndex];
    if (row[0] !== stall || row[1] !== pits || row[2] < 5 || row[2] > 100
        || row[3] !== cycles || row[4] !== delta || row[5] !== stat || row[6] !== counter) {
      throw new Error(`Malformed Android frame gameplay row ${caseIndex + 1}: ${Array.from(row).join(' ')}`);
    }
    for (let cycle = 0; cycle < times.length; cycle++) {
      if (row[7 + cycle] !== times[cycle]) {
        throw new Error(`Android frame gameplay case ${caseIndex + 1}, cycle ${cycle + 1} differs: ${row[7 + cycle]} != ${times[cycle]}`);
      }
    }
  }
  console.log('Android 60 Hz frame/tick integration: production GamePlayLoop timestamps match Pascal GAMESCHD for normal speed 6 and a 300 ms frame stall after poll 2');
  const gameStartEnterResult = instance.exports.run_game_start_enter_checks();
  if (gameStartEnterResult !== 0) throw new Error(`Game-start board-entry check returned ${gameStartEnterResult}`);
  console.log('Game start: board-entry fields and the continuous Pascal clock/counter are preserved; title starts unpaused and play starts paused');

  const windowModule = await WebAssembly.compile(fs.readFileSync(process.argv[3]));
  const windowInstance = await WebAssembly.instantiate(windowModule, {});
  const windowResult = windowInstance.exports.run_window_label_checks();
  if (windowResult !== 0) throw new Error(`Window label check returned ${windowResult}`);
  console.log('Text-window links: case-insensitive label prefixes jump in-document; missing labels close; selection links are preserved');

  const reloadModule = await WebAssembly.compile(fs.readFileSync(process.argv[4]));
  // zoo_world_play's success-only continuation is linked from zoo_game.c.
  // This fixture deliberately takes the missing-file return before those
  // imported success-path services can be called.
  const reloadEnv = new Proxy({}, { get: () => () => 0 });
  const reloadInstance = await WebAssembly.instantiate(reloadModule, { env: reloadEnv });
  const reloadResult = reloadInstance.exports.run_world_reload_checks();
  if (reloadResult !== 0) throw new Error(`World reload check returned ${reloadResult}`);
  console.log('Save reload: empty and embedded-NUL names follow Pascal path truncation; CP437 bytes and case survive; missing files fail and handles close');

  const soundModule = await WebAssembly.compile(fs.readFileSync(process.argv[5]));
  const soundInstance = await WebAssembly.instantiate(soundModule, {});
  const soundResult = soundInstance.exports.run_sound_parse_checks();
  if (soundResult !== 0) throw new Error(`Sound parser check returned ${soundResult}`);
  const soundExports = soundInstance.exports;
  const soundInputPtr = soundExports.sound_input_ptr();
  const soundOutputPtr = soundExports.sound_output_ptr();
  const alphabet = 'ABCDEFGabcdefg!#TtSsIiQqHhWw.+-3Xx012456789 ';
  let randomState = 0x5a17c9e3;
  const nextRandom = () => {
    randomState = (Math.imul(randomState, 1664525) + 1013904223) >>> 0;
    return randomState;
  };
  const cases = ['C!', 'B#', 'Q..........C', 'W....................C'];
  for (let test = 0; test < 5000; test++) {
    const length = nextRandom() % 81;
    let input = '';
    for (let i = 0; i < length; i++) input += alphabet[nextRandom() % alphabet.length];
    cases.push(input);
  }
  for (const input of cases) {
    const bytes = Buffer.from(input, 'ascii');
    new Uint8Array(soundInstance.exports.memory.buffer, soundInputPtr, bytes.length).set(bytes);
    const actualLength = soundExports.run_sound_parse_from_input(bytes.length);
    if (actualLength < 0) throw new Error(`Sound parser rejected input of length ${bytes.length}`);
    const actual = Buffer.from(new Uint8Array(soundInstance.exports.memory.buffer,
      soundOutputPtr, actualLength).slice());
    const expected = pascalSoundParse(input);
    if (!actual.equals(expected)) {
      throw new Error(`Sound parser differs from Pascal model for ${JSON.stringify(input)}: ${actual.toString('hex')} != ${expected.toString('hex')}`);
    }
  }
  console.log(`Sound parser: ${cases.length} deterministic inputs match Pascal semantics, including flats and 16-bit duration overflow`);

  const soundTickModule = await WebAssembly.compile(fs.readFileSync(process.argv[20]));
  const soundTickInstance = await WebAssembly.instantiate(soundTickModule, {});
  const soundTickResult = soundTickInstance.exports.run_sound_tick_checks();
  if (soundTickResult !== 0) throw new Error(`Sound tick check returned ${soundTickResult}`);
  console.log('Sound tick: Pascal byte-boundary stops and zero-duration 256-tick wrap match the production scheduler');
  const soundPitchResult = soundTickInstance.exports.run_sound_pitch_table_checks();
  if (soundPitchResult !== 0) throw new Error(`Sound pitch-table check returned ${soundPitchResult}`);
  const soundPitchPtr = soundTickInstance.exports.sound_pitch_output_ptr();
  const soundPitchTable = new Uint16Array(soundTickInstance.exports.memory.buffer, soundPitchPtr, 72);
  if (soundPitchTable[0] !== 64 || soundPitchTable[11] !== 120
      || soundPitchTable[12] !== 128 || soundPitchTable[71] !== 3866) {
    throw new Error('Sound pitch-table capture did not return the 72 supported Pascal note frequencies');
  }
  console.log('Sound pitch table: production note dispatch captures all 72 supported octave/note positions');

  const oopModule = await WebAssembly.compile(fs.readFileSync(process.argv[6]));
  const oopInstance = await WebAssembly.instantiate(oopModule, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const oopResult = oopInstance.exports.run_oop_label_checks();
  if (oopResult !== 0) throw new Error(`OOP label check returned ${oopResult}`);
  console.log('ZZT-OOP labels: object names and messages honor Pascal string[20] truncation and reject longer label prefixes');

  const oopFlagResult = oopInstance.exports.run_oop_flag_checks();
  if (oopFlagResult !== 0) throw new Error(`OOP flag check returned ${oopFlagResult}`);
  console.log('ZZT-OOP flags: short-string lengths, last-match lookup, empty-name result, clear order, and empty-slot selection match Pascal');

  const oopWindowTitleResult = oopInstance.exports.run_oop_window_title_checks();
  if (oopWindowTitleResult !== 0) throw new Error(`OOP window-title check returned ${oopWindowTitleResult}`);
  console.log('ZZT-OOP text windows: caller default title survives stack reuse, @ titles override it, and empty @ falls back to Interaction');

  const oopTryCommandResult = oopInstance.exports.run_oop_try_command_checks();
  if (oopTryCommandResult !== 0) throw new Error(`OOP TRY command check returned ${oopTryCommandResult}`);
  console.log('ZZT-OOP TRY: a blocked move continues with the next command; a successful move ends the instruction');

  const oopIfResult = oopInstance.exports.run_oop_if_condition_checks();
  if (oopIfResult !== 0) throw new Error(`OOP IF condition check returned ${oopIfResult}`);
  console.log('ZZT-OOP IF: 55 Pascal conditions match, including recursive NOT, aligned/contact/blocked, ANY, and flag flow');

  const oopCounterResult = oopInstance.exports.run_oop_counter_overflow_checks();
  if (oopCounterResult !== 0) throw new Error(`OOP counter-overflow check returned ${oopCounterResult}`);
  console.log('ZZT-OOP GIVE/TAKE: signed 16-bit boundaries for all six counters and same-line flow match Pascal');

  const oopZapRestoreResult = oopInstance.exports.run_oop_zap_restore_checks();
  if (oopZapRestoreResult !== 0) throw new Error(`OOP ZAP/RESTORE check returned ${oopZapRestoreResult}`);
  console.log('ZZT-OOP ZAP/RESTORE: directives mutate labels in place and restore repeated zapped labels');

  const oopBindSelfResult = oopInstance.exports.run_oop_bind_self_checks();
  if (oopBindSelfResult !== 0) throw new Error(`OOP BIND SELF check returned ${oopBindSelfResult}`);
  console.log('ZZT-OOP BIND: self/cross-object cases and two shared-alias follower calls match direct Pascal traces for the tested layouts');

  const oopBindSelectorResult = oopInstance.exports.run_oop_bind_selector_checks();
  if (oopBindSelectorResult !== 0) throw new Error(`OOP BIND selector check returned ${oopBindSelectorResult}`);
  console.log('ZZT-OOP BIND selectors: OTHERS from first and later stats, ALL, first case-insensitive named match, and missing-target continuation match direct Pascal traces');

  const oopFlowControlResult = oopInstance.exports.run_oop_flow_control_checks();
  if (oopFlowControlResult !== 0) throw new Error(`OOP flow-control check returned ${oopFlowControlResult}`);
  console.log('ZZT-OOP flow control: IDLE defers, END stops, ENDGAME continues, RESTART resets, GO retries blocked moves, and LOCK/UNLOCK mutate P2');

  const oopObjectCommandResult = oopInstance.exports.run_oop_object_command_checks();
  if (oopObjectCommandResult !== 0) throw new Error(`OOP object-command check returned ${oopObjectCommandResult}`);
  console.log('ZZT-OOP object commands: BECOME, DIE, PUT bounds/color, CHANGE scans, CHAR, and WALK match dispatcher behavior');

  const oopCharResult = oopInstance.exports.run_oop_char_capture_checks();
  if (oopCharResult !== 0) throw new Error(`OOP #CHAR capture returned ${oopCharResult}`);
  const oopCharValues = [-32768, -1, 0, 1, 127, 255, 256, 32767];
  const oopCharPointer = oopInstance.exports.oop_char_output_ptr();
  const oopCharRows = new Int16Array(oopInstance.exports.memory.buffer, oopCharPointer, 32);
  oopCharValues.forEach((value, index) => {
    const expectedGlyph = value >= 1 && value <= 255 ? value : 0xA5;
    if (oopCharRows[index * 4] !== index + 1
      || oopCharRows[index * 4 + 1] !== expectedGlyph
      || oopCharRows[index * 4 + 2] !== 3) {
      throw new Error(`OOP #CHAR boundary check failed for ${value}: ${Array.from(oopCharRows.slice(index * 4, index * 4 + 4)).join(' ')}`);
    }
  });
  console.log('ZZT-OOP #CHAR: 0/1/255/256 and signed extremes preserve or replace P1 correctly; same-line command remains skipped');

  const oopPutResult = oopInstance.exports.run_oop_put_capture_checks();
  if (oopPutResult !== 0) throw new Error(`OOP #PUT capture returned ${oopPutResult}`);
  const oopPutPointer = oopInstance.exports.oop_put_output_ptr();
  const oopPutRows = new Int16Array(oopInstance.exports.memory.buffer, oopPutPointer, 80);
  const oopPutAllowed = new Set([0, 2, 4, 7]);
  for (let index = 0; index < 8; index++) {
    const allowed = oopPutAllowed.has(index);
    const expectedElement = allowed ? 21 : (index === 3 || index === 5 ? 36 : 0);
    const expectedColor = allowed ? 0x2A : (index === 3 || index === 5 ? 0x0F : 0x2A);
    if (oopPutRows[index * 10] !== index + 1
      || oopPutRows[index * 10 + 5] !== expectedElement
      || oopPutRows[index * 10 + 6] !== expectedColor
      || oopPutRows[index * 10 + 7] !== 3) {
      throw new Error(`OOP #PUT edge check failed for case ${index + 1}: ${Array.from(oopPutRows.slice(index * 10, index * 10 + 10)).join(' ')}`);
    }
  }
  console.log('ZZT-OOP #PUT: asymmetric top/bottom and left/right boundary rules match the original board edges');

  const oopChangeResult = oopInstance.exports.run_oop_change_capture_checks();
  if (oopChangeResult !== 0) throw new Error(`OOP #CHANGE capture returned ${oopChangeResult}`);
  const oopChangePointer = oopInstance.exports.oop_change_output_ptr();
  const oopChangeMap = new Uint8Array(oopInstance.exports.memory.buffer, oopChangePointer, 61 * 26 * 2);
  let oopChangeCount = 0;
  for (let y = 0; y <= 25; y++) {
    for (let x = 0; x <= 60; x++) {
      const index = (y * 61 + x) * 2;
      const originalColor = (x * 31 + y * 17) & 0xFF;
      let expectedElement = 0;
      let expectedColor = originalColor;
      if (x === 10 && y === 10) {
        expectedElement = 36;
        expectedColor = 0x1F;
      } else if (x === 11 && y === 10) {
        expectedElement = 4;
        expectedColor = 0x0F;
      } else if (x === 20 && y === 20) {
        expectedElement = 21;
        expectedColor = 0x3C;
      } else if (x > 0 && y > 0) {
        expectedElement = 21;
        expectedColor = originalColor || 0x0F;
        oopChangeCount++;
      }
      if (oopChangeMap[index] !== expectedElement || oopChangeMap[index + 1] !== expectedColor) {
        throw new Error(`OOP #CHANGE map mismatch at ${x},${y}: ${oopChangeMap[index]}/${oopChangeMap[index + 1]} != ${expectedElement}/${expectedColor}`);
      }
    }
  }
  if (oopChangeCount !== 1497) throw new Error(`OOP #CHANGE expected to change 1497 cells, got ${oopChangeCount}`);
  console.log('ZZT-OOP #CHANGE: all 1497 matching cells, edge scan limits, nonmatching elements, and retained colors match Pascal');

  const oopFlagAndSendResult = oopInstance.exports.run_oop_flag_and_send_command_checks();
  if (oopFlagAndSendResult !== 0) throw new Error(`OOP flag/SEND command check returned ${oopFlagAndSendResult}`);
  console.log('ZZT-OOP SET/CLEAR/SEND: flags mutate; self and cross-object labels dispatch with target locks honored');

  const oopSendCaptureResult = oopInstance.exports.run_oop_send_capture_checks();
  if (oopSendCaptureResult !== 0) throw new Error(`OOP SEND capture returned ${oopSendCaptureResult}`);
  console.log('ZZT-OOP SEND: self/event lock rules, duplicate-target iteration, parsed self-dispatch, and cross-object recipient execution match');

  const oopFlagCaptureResult = oopInstance.exports.run_oop_flag_state_capture_checks();
  if (oopFlagCaptureResult !== 0) throw new Error(`OOP flag-state capture returned ${oopFlagCaptureResult}`);
  const oopFlagPointer = oopInstance.exports.oop_flag_capture_output_ptr();
  const oopFlagBytes = new Uint8Array(oopInstance.exports.memory.buffer, oopFlagPointer, 434);
  const oopFlagDataView = new DataView(oopInstance.exports.memory.buffer);
  const firstFlagNames = ['ALPHA', 'BETA', ''];
  const firstFlagLengths = [5, 4, 0];
  for (let slot = 0; slot < firstFlagLengths.length; slot++) {
    const offset = slot * 21;
    const actualName = String.fromCharCode(...oopFlagBytes.slice(offset + 1, offset + 1 + firstFlagLengths[slot]));
    if (oopFlagBytes[offset] !== firstFlagLengths[slot] || actualName !== firstFlagNames[slot]) {
      throw new Error(`OOP flag duplicate-clear/slot-reuse mismatch in slot ${slot + 1}: ${oopFlagBytes[offset]} ${actualName}`);
    }
  }
  const firstFlagIdOffset = 210;
  if (oopFlagDataView.getInt16(oopFlagPointer + firstFlagIdOffset, true) !== 0
    || oopFlagDataView.getInt16(oopFlagPointer + firstFlagIdOffset + 2, true) !== -1
    || oopFlagDataView.getInt16(oopFlagPointer + firstFlagIdOffset + 4, true) !== 1
    || oopFlagDataView.getInt16(oopFlagPointer + firstFlagIdOffset + 6, true) !== -1) {
    throw new Error('OOP flag lookup IDs differ after duplicate clearing and slot reuse');
  }
  const fullFlagOffset = 218;
  const finalFlagOffset = fullFlagOffset + 9 * 21;
  if (oopFlagBytes[finalFlagOffset] !== 8
    || String.fromCharCode(...oopFlagBytes.slice(finalFlagOffset + 1, finalFlagOffset + 9)) !== 'OVERFLOW'
    || oopFlagDataView.getInt16(oopFlagPointer + 428 + 4, true) !== 9) {
    throw new Error('OOP full-table #SET did not replace the final Pascal flag slot');
  }
  console.log('ZZT-OOP flags: duplicate #CLEAR, first-empty-slot #SET, stale short-string tails, and full-table overwrite captured for Pascal comparison');

  const oopCombatAndPlayResult = oopInstance.exports.run_oop_combat_and_play_checks();
  if (oopCombatAndPlayResult !== 0) throw new Error(`OOP combat/PLAY command check returned ${oopCombatAndPlayResult}`);
  console.log('ZZT-OOP SHOOT/THROWSTAR/PLAY: projectile request directions and stop flow, plus production shot and tune queues');

  const oopDirectionResult = oopInstance.exports.run_oop_direction_parser_checks();
  if (oopDirectionResult !== 0) throw new Error(`OOP direction parser check returned ${oopDirectionResult}`);
  console.log('ZZT-OOP directions: aliases, flow/seek, recursive turns, random modifiers, and invalid tokens match Pascal');

  const oopTileResult = oopInstance.exports.run_oop_tile_parser_checks();
  if (oopTileResult !== 0) throw new Error(`OOP tile parser check returned ${oopTileResult}`);
  console.log('ZZT-OOP tiles: normalized element names, all seven colors, color case folding, and failed-color retention match Pascal');

  const oopPlaceTileResult = oopInstance.exports.run_oop_place_tile_checks();
  if (oopPlaceTileResult !== 0) throw new Error(`OOP place-tile check returned ${oopPlaceTileResult}`);
  console.log('ZZT-OOP tile placement: Pascal byte wrapping for white-on-choice colors matches across all 256 prior tile attribute bytes');

  const oopLineReaderResult = oopInstance.exports.run_oop_line_reader_checks();
  if (oopLineReaderResult !== 0) throw new Error(`OOP line-reader check returned ${oopLineReaderResult}`);
  console.log('ZZT-OOP line reader: lengths 0 through 255 and a 300-byte truncation preserve CR cursor positions');

  const oopValuePtr = oopInstance.exports.oop_value_input_ptr();
  const oopWordResultPtr = oopInstance.exports.oop_word_result_ptr();
  const oopBytes = (input) => Buffer.isBuffer(input) ? input : Buffer.from(input, 'ascii');
  const pascalUpCaseByte = (byte) => byte >= 97 && byte <= 122 ? byte - 32 : byte;
  const oopWordModel = (input) => {
    const bytes = oopBytes(input);
    let position = 0;
    const readChar = () => position < bytes.length ? bytes[position++] : 0;
    let character = readChar();
    while (character === 32) character = readChar();
    character = pascalUpCaseByte(character);
    const word = [];
    if (character < 48 || character > 57) {
      while ((character >= 65 && character <= 90) || character === 58
          || (character >= 48 && character <= 57) || character === 95) {
        if (word.length < 20) word.push(character);
        character = pascalUpCaseByte(readChar());
      }
    }
    if (position > 0) position--;
    return { word: Buffer.from(word), position };
  };
  const oopWordCases = ['7MOVE\r', '4ABC\r', 'A1\r', '  NORTH\r', '_A:2\r',
    'ABCDEFGHIJKLMNOPQRSTUV\r', ''].map((input) => Buffer.from(input, 'ascii'));
  for (let byte = 0; byte < 256; byte++) {
    oopWordCases.push(Buffer.from([byte]));
    oopWordCases.push(Buffer.from([32, byte, 65, 49, 33]));
    oopWordCases.push(Buffer.from([65, byte, 66]));
  }
  for (const input of oopWordCases) {
    const bytes = oopBytes(input);
    new Uint8Array(oopInstance.exports.memory.buffer, oopValuePtr, bytes.length).set(bytes);
    const actualPosition = oopInstance.exports.run_oop_word_from_input(bytes.length);
    const wordBytes = new Uint8Array(oopInstance.exports.memory.buffer, oopWordResultPtr, 21);
    let wordLength = wordBytes.indexOf(0);
    if (wordLength < 0) wordLength = wordBytes.length;
    const actualWord = Buffer.from(wordBytes.slice(0, wordLength));
    const expected = oopWordModel(bytes);
    if (!actualWord.equals(expected.word) || actualPosition !== expected.position) {
      throw new Error(`OOP word parser differs from Pascal model for ${bytes.toString('hex')}: ` +
        `${actualWord.toString('hex')}@${actualPosition} != ${expected.word.toString('hex')}@${expected.position}`);
    }
  }
  console.log(`ZZT-OOP words: ${oopWordCases.length} Pascal-model inputs match digit-leading cursor rewind and string[20] truncation`);

  // A direct TP 5.5 Val(Integer) probe returns 0/code 10 above signed LongInt;
  // OopReadValue ignores the code, so its result is 0 rather than the prior value.
  const oopValueModel = (input) => {
    const bytes = oopBytes(input);
    let position = 0;
    const readChar = () => position < bytes.length ? bytes[position++] : 0;
    let character = readChar();
    while (character === 32) character = readChar();
    const digits = [];
    while (character >= 48 && character <= 57) {
      if (digits.length < 20) digits.push(String.fromCharCode(character));
      character = readChar();
    }
    if (position > 0) position--;
    const parsed = digits.length ? BigInt(digits.join('')) : 0n;
    return { value: digits.length ? (parsed > 2147483647n ? 0 : Number(BigInt.asIntN(16, parsed))) : -1, position };
  };
  const oopValueCases = ['', 'x', '-9', ' 42,', '32767 ', '32768,', '65535?',
    '2147483647!', '2147483648!', '2147483649?', '99999999999999999999',
    '0000000000000000000042#'].map((input) => Buffer.from(input, 'ascii'));
  for (let byte = 0; byte < 256; byte++) {
    oopValueCases.push(Buffer.from([byte]));
    oopValueCases.push(Buffer.from([32, byte, 55, 33]));
  }
  let oopValueRandom = 0x6d2b79f5;
  for (let test = 0; test < 512; test++) {
    oopValueRandom = (Math.imul(oopValueRandom, 1664525) + 1013904223) >>> 0;
    const number = oopValueRandom % 2147483648;
    oopValueRandom = (Math.imul(oopValueRandom, 1664525) + 1013904223) >>> 0;
    const padding = oopValueRandom % 9;
    oopValueCases.push(Buffer.from(`${'0'.repeat(padding)}${number}!`, 'ascii'));
  }
  for (const input of oopValueCases) {
    const bytes = oopBytes(input);
    new Uint8Array(oopInstance.exports.memory.buffer, oopValuePtr, bytes.length).set(bytes);
    const actualValue = oopInstance.exports.run_oop_value_from_input(bytes.length, 1234);
    const actualPosition = oopInstance.exports.oop_value_result_position();
    const expected = oopValueModel(input);
    if (actualValue !== expected.value || actualPosition !== expected.position) {
      throw new Error(`OOP numeric parser differs from Pascal model for ${bytes.toString('hex')}: ` +
        `${actualValue}@${actualPosition} != ${expected.value}@${expected.position}`);
    }
  }
  console.log(`ZZT-OOP numeric values: ${oopValueCases.length} Pascal-model inputs match short-string truncation, cursor position, and 16-bit overflow`);

  const rngModule = await WebAssembly.compile(fs.readFileSync(process.argv[7]));
  const rngInstance = await WebAssembly.instantiate(rngModule, {});
  const rngResult = rngInstance.exports.run_rng_checks();
  if (rngResult !== 0) throw new Error(`Turbo Pascal RNG check returned ${rngResult}`);
  console.log('Turbo Pascal 5.5 RNG: seed-123 sequence, DOS time packing, seed-zero drum values, and zero-range advance match');

  const playerInputModule = await WebAssembly.compile(fs.readFileSync(process.argv[8]));
  const playerInputInstance = await WebAssembly.instantiate(playerInputModule, {});
  const playerInputResult = playerInputInstance.exports.run_player_input_checks();
  if (playerInputResult !== 0) throw new Error(`Player input check returned ${playerInputResult}`);
  console.log('Player input: Pascal burst-buffer truncation, queued arrow bursts, CP437 extended-byte unwrapping, queued Space after death, and held/tap firing behavior match');

  const spinningGunModule = await WebAssembly.compile(fs.readFileSync(process.argv[9]));
  const spinningGunInstance = await WebAssembly.instantiate(spinningGunModule, {});
  const spinningGunResult = spinningGunInstance.exports.run_spinning_gun_checks();
  if (spinningGunResult !== 0) throw new Error(`Spinning-gun check returned ${spinningGunResult}`);
  console.log('Spinning gun: aimed/random shot requests, TP 5.5 RNG advances, Bullet/Star selection, and same-row fallback match Pascal');

  const blinkWallModule = await WebAssembly.compile(fs.readFileSync(process.argv[10]));
  const blinkWallInstance = await WebAssembly.instantiate(blinkWallModule, {});
  const blinkWallResult = blinkWallInstance.exports.run_blink_wall_compat_checks();
  if (blinkWallResult !== 0) throw new Error(`Blink-wall compatibility check returned ${blinkWallResult}`);
  console.log('Blink wall: vertical player push preserves Pascal’s left-check/right-move fallback quirk');

  const directionSeekModule = await WebAssembly.compile(fs.readFileSync(process.argv[11]));
  const directionSeekInstance = await WebAssembly.instantiate(directionSeekModule, {});
  const directionSeekResult = directionSeekInstance.exports.run_direction_seek_checks();
  if (directionSeekResult !== 0) throw new Error(`Direction-seeking compatibility check returned ${directionSeekResult}`);
  console.log('Direction seek: production helper matches Pascal alignment, random-axis, and energizer-reversal cases');

  const boardEdgeModule = await WebAssembly.compile(fs.readFileSync(process.argv[12]));
  const boardEdgeInstance = await WebAssembly.instantiate(boardEdgeModule, {});
  const boardEdgeResult = boardEdgeInstance.exports.run_board_edge_compat_checks();
  if (boardEdgeResult !== 0) throw new Error(`Board-edge compatibility check returned ${boardEdgeResult}`);
  console.log('Board edge: all four neighbor directions preserve horizontal rows and match Pascal entry, blocked rollback, and existing-player cases');

  const transporterModule = await WebAssembly.compile(fs.readFileSync(process.argv[13]));
  const transporterInstance = await WebAssembly.instantiate(transporterModule, {});
  const transporterResult = transporterInstance.exports.run_transporter_compat_checks();
  if (transporterResult !== 0) throw new Error(`Transporter compatibility check returned ${transporterResult}`);
  console.log('Transporter: matching/reverse-facing routes, wrong-way stops, blocked exits, and exact priority-3 sound bytes match Pascal');

  const soundPcmModule = await WebAssembly.compile(fs.readFileSync(process.argv[14]));
  const soundPcmInstance = await WebAssembly.instantiate(soundPcmModule, {});
  const soundPcmResult = soundPcmInstance.exports.run_sound_pcm_checks();
  if (soundPcmResult !== 0) throw new Error(`Sound PCM check returned ${soundPcmResult}`);
  console.log('Sound PCM: zero-frequency PC-speaker drum steps stay silent; 440 Hz tone keeps the expected square-wave phase');
  const playerGlyphModule = await WebAssembly.compile(fs.readFileSync(process.argv[15]));
  const playerGlyphInstance = await WebAssembly.instantiate(playerGlyphModule, {});
  const playerGlyphResult = playerGlyphInstance.exports.run_player_glyph_checks();
  if (playerGlyphResult !== 0) throw new Error(`Player glyph check returned ${playerGlyphResult}`);
  console.log('Player glyph: energized frames toggle like Pascal and redraws preserve the current glyph');
  const windowFileHintsModule = await WebAssembly.compile(fs.readFileSync(process.argv[16]));
  const windowFileHintsInstance = await WebAssembly.instantiate(windowFileHintsModule, {});
  const windowFileHintsResult = windowFileHintsInstance.exports.run_window_file_hint_checks();
  if (windowFileHintsResult !== 0) throw new Error(`Window file-hint check returned ${windowFileHintsResult}`);
  console.log('Text window: external-file prompts match Pascal CP437 glyphs, positions, colors, and untouched cells');
  const monitorModule = await WebAssembly.compile(fs.readFileSync(process.argv[17]));
  const monitorInstance = await WebAssembly.instantiate(monitorModule, {});
  const monitorResult = monitorInstance.exports.run_monitor_compat_checks();
  if (monitorResult !== 0) throw new Error(`Monitor compatibility check returned ${monitorResult}`);
  console.log('Monitor and arrow keys: Pascal exit handling, case folding, one-key buffering, and arrow-key movement mapping match');

  const boardShootModule = await WebAssembly.compile(fs.readFileSync(process.argv[18]));
  const boardShootInstance = await WebAssembly.instantiate(boardShootModule, {
    env: new Proxy({}, { get: () => () => 0 })
  });
  const boardShootResult = boardShootInstance.exports.run_board_shoot_checks();
  if (boardShootResult !== 0) throw new Error(`Board-shoot compatibility check returned ${boardShootResult}`);
  console.log('Board shoot: production helper creates bullets/stars on walkable/water cells and preserves source, damage, and energizer rules');

  const oopMovementModule = await WebAssembly.compile(fs.readFileSync(process.argv[19]));
  let oopMovementInstance;
  let oopHeapTop = 8 * 1024 * 1024;
  const oopAllocations = new Map();
  const oopMalloc = size => {
    const length = Math.max(1, Number(size) >>> 0);
    const pointer = oopHeapTop;
    oopHeapTop = (oopHeapTop + length + 7) & ~7;
    if (oopHeapTop > 15 * 1024 * 1024) return 0;
    oopAllocations.set(pointer, length);
    return pointer;
  };
  const oopEnvironment = new Proxy({}, {
    get: (_target, name) => {
      if (name === 'malloc') return oopMalloc;
      if (name === 'free') return pointer => { oopAllocations.delete(Number(pointer)); };
      if (name === 'realloc') return (pointer, size) => {
        pointer = Number(pointer);
        const length = Math.max(1, Number(size) >>> 0);
        if (!pointer) return oopMalloc(length);
        const oldLength = oopAllocations.get(pointer) || 0;
        if (length <= oldLength) {
          oopAllocations.set(pointer, length);
          return pointer;
        }
        const nextPointer = oopMalloc(length);
        if (!nextPointer) return 0;
        if (oopMovementInstance) {
          new Uint8Array(oopMovementInstance.exports.memory.buffer)
            .copyWithin(nextPointer, pointer, pointer + oldLength);
        }
        return nextPointer;
      };
      return () => 0;
    }
  });
  oopMovementInstance = await WebAssembly.instantiate(oopMovementModule, { env: oopEnvironment });
  const oopMovementResult = oopMovementInstance.exports.run_oop_production_movement_checks();
  if (oopMovementResult !== 0) throw new Error(`OOP production-movement check returned ${oopMovementResult}`);
  const bindReuseResult = oopMovementInstance.exports.run_oop_bind_reuse_addstat_checks();
  if (bindReuseResult !== 0) throw new Error(`OOP #BIND AddStat reuse check returned ${bindReuseResult}`);
	console.log('ZZT-OOP BIND: AddStat matches eleven measured allocation sizes, contiguous tail splits, stale-alias bytes, and the 26/27/28-byte reuse transition');
  const bindTextWindowResult = oopMovementInstance.exports.run_oop_bind_text_window_reuse_checks();
  if (bindTextWindowResult !== 0) throw new Error(`OOP #BIND TextWindow allocation check returned ${bindTextWindowResult}`);
  console.log('ZZT-OOP #BIND: Pascal TextWindow short-string allocations reuse and overwrite aliased script blocks at the measured 50/51-byte boundary');
  const slimeCaptureResult = oopMovementInstance.exports.run_slime_tick_capture_checks();
  if (slimeCaptureResult !== 0) throw new Error(`Slime tick capture returned ${slimeCaptureResult}`);
  const bombTickCaptureResult = oopMovementInstance.exports.run_bomb_tick_capture_checks();
  if (bombTickCaptureResult !== 0) throw new Error(`Bomb tick capture returned ${bombTickCaptureResult}`);
  const conveyorTickCaptureResult = oopMovementInstance.exports.run_conveyor_tick_capture_checks();
  if (conveyorTickCaptureResult !== 0) throw new Error(`Conveyor tick capture returned ${conveyorTickCaptureResult}`);
  const starTickCaptureResult = oopMovementInstance.exports.run_star_tick_capture_checks();
  if (starTickCaptureResult !== 0) throw new Error(`Star tick capture returned ${starTickCaptureResult}`);
  const tigerTickCaptureResult = oopMovementInstance.exports.run_tiger_tick_capture_checks();
  if (tigerTickCaptureResult !== 0) throw new Error(`Tiger tick capture returned ${tigerTickCaptureResult}`);
  const centipedeTickCaptureResult = oopMovementInstance.exports.run_centipede_tick_capture_checks();
  if (centipedeTickCaptureResult !== 0) throw new Error(`Centipede tick capture returned ${centipedeTickCaptureResult}`);
  const elementDrawCaptureResult = oopMovementInstance.exports.run_element_draw_capture_checks();
  if (elementDrawCaptureResult !== 0) throw new Error(`Element draw capture returned ${elementDrawCaptureResult}`);
  const objectTickCaptureResult = oopMovementInstance.exports.run_oop_object_tick_capture_checks();
  if (objectTickCaptureResult !== 0) throw new Error(`Object tick capture returned ${objectTickCaptureResult}`);
  const objectTouchCaptureResult = oopMovementInstance.exports.run_oop_object_touch_capture_checks();
  if (objectTouchCaptureResult !== 0) throw new Error(`Object touch capture returned ${objectTouchCaptureResult}`);
  const messageTimerCaptureResult = oopMovementInstance.exports.run_message_timer_capture_checks();
  if (messageTimerCaptureResult !== 0) throw new Error(`Message timer capture returned ${messageTimerCaptureResult}`);
  const ruffianTickCaptureResult = oopMovementInstance.exports.run_ruffian_tick_capture_checks();
  if (ruffianTickCaptureResult !== 0) throw new Error(`Ruffian tick capture returned ${ruffianTickCaptureResult}`);
  const bearTickCaptureResult = oopMovementInstance.exports.run_bear_tick_capture_checks();
  if (bearTickCaptureResult !== 0) throw new Error(`Bear tick capture returned ${bearTickCaptureResult}`);
  const sharkTickCaptureResult = oopMovementInstance.exports.run_shark_tick_capture_checks();
  if (sharkTickCaptureResult !== 0) throw new Error(`Shark tick capture returned ${sharkTickCaptureResult}`);
  const lionTickCaptureResult = oopMovementInstance.exports.run_lion_tick_capture_checks();
  if (lionTickCaptureResult !== 0) throw new Error(`Lion tick capture returned ${lionTickCaptureResult}`);
  console.log('ZZT elements: TRY/GO pushes, pusher chains, sliders, duplicator, Object TOUCH event targeting/lock, Scroll default title and @ override, player torch touch/tick, fire, movement, P-key pause/timer order, B-key sound-toggle/queue behavior, paused move/unpause/save/no-double-step, and death ticks, centipede chain following/reversal/color bytes and segment-to-head delay, Bear chase priority/range, Shark water-only movement, Lion direction branches/random order, Ruffian wake/movement/stop thresholds and tile-color transitions, Tiger shot fallback/star selection, Star countdown/movement/removal, Energizer timer/message/sound/OOP event, Torch pickup state, invisible/forest/fake/water touch behavior, passage board changes and last matching color selection, shared player damage health edges, slime touch and sound priority, active-play gem health clamp after +1, Pascal black-key/Gems alias, Black door, one-based colored key/door mapping, bullet ricochets, conveyor turns, Bomb activation/push/countdown, and slime reproduction intervals match Pascal');

  const gameClockModule = await WebAssembly.compile(fs.readFileSync(process.argv[21]));
  const gameClockInstance = await WebAssembly.instantiate(gameClockModule, {});
  const gameClockResult = gameClockInstance.exports.run_game_clock_checks();
  if (gameClockResult !== 0) throw new Error(`Production game-clock check returned ${gameClockResult}`);
  const gameClockOutput = new Int16Array(gameClockInstance.exports.memory.buffer,
    gameClockInstance.exports.game_clock_output_ptr(), 14);
  const expectedClockOutput = [
    0, 5990, 1, 5999, 1, 1, 0, 1, 1, 6, 1, 1, 1, 90
  ];
  for (let i = 0; i < expectedClockOutput.length; i++) {
    if (gameClockOutput[i] !== expectedClockOutput[i]) {
      throw new Error(`Production elapsed-clock case ${i + 1} differs: ${gameClockOutput[i]} != ${expectedClockOutput[i]}`);
    }
  }
  console.log('Game elapsed clock: zero-duration success, minute rollover, exact thresholds, retained counters, and modulo clock-step behavior match the Pascal contract');

  const integratedModule = await WebAssembly.compile(fs.readFileSync(process.argv[22]));
  let integratedInstance;
  let integratedHeapTop = 8 * 1024 * 1024;
  const integratedAllocations = new Map();
  const integratedMalloc = size => {
    const length = Math.max(1, Number(size) >>> 0);
    const pointer = integratedHeapTop;
    integratedHeapTop = (integratedHeapTop + length + 7) & ~7;
    if (integratedHeapTop > 15 * 1024 * 1024) return 0;
    integratedAllocations.set(pointer, length);
    return pointer;
  };
  const integratedEnvironment = new Proxy({}, {
    get: (_target, name) => {
      if (name === 'malloc') return integratedMalloc;
      if (name === 'free') return pointer => { integratedAllocations.delete(Number(pointer)); };
      if (name === 'realloc') return (pointer, size) => {
        pointer = Number(pointer);
        const length = Math.max(1, Number(size) >>> 0);
        if (!pointer) return integratedMalloc(length);
        const oldLength = integratedAllocations.get(pointer) || 0;
        if (length <= oldLength) {
          integratedAllocations.set(pointer, length);
          return pointer;
        }
        const nextPointer = integratedMalloc(length);
        if (!nextPointer) return 0;
        if (integratedInstance) new Uint8Array(integratedInstance.exports.memory.buffer).copyWithin(nextPointer, pointer, pointer + oldLength);
        return nextPointer;
      };
      return () => 0;
    }
  });
  integratedInstance = await WebAssembly.instantiate(integratedModule, { env: integratedEnvironment });
  const integratedResult = integratedInstance.exports.run_integrated_gameplay_checks();
  if (integratedResult !== 0) {
    const trace = new Int16Array(integratedInstance.exports.memory.buffer,
      integratedInstance.exports.integrated_gameplay_trace_ptr(), 24 * 9);
    throw new Error(`Integrated gameplay replay returned ${integratedResult}; trace=${Array.from(trace).join(' ')}`);
  }
  const integratedFieldCount = integratedInstance.exports.integrated_gameplay_output_length();
  const integratedOutput = new Int32Array(integratedInstance.exports.memory.buffer,
    integratedInstance.exports.integrated_gameplay_output_ptr(), integratedFieldCount);
  const integratedSchedulerStart = 13 + (integratedOutput[11] + 1) * 14;
  const integratedPaused = integratedOutput[integratedSchedulerStart + 5];
  const integratedExitRequested = integratedOutput[integratedSchedulerStart + 6];
  if (integratedOutput[0] !== 1 || integratedOutput[1] !== 1 || integratedOutput[11] !== 2
      || integratedPaused !== 0 || integratedExitRequested !== 1) {
    throw new Error(`Integrated gameplay invariant failed: ammo=${integratedOutput[0]}, gems=${integratedOutput[1]}, stats=${integratedOutput[11]}, paused=${integratedPaused}, exit=${integratedExitRequested}`);
  }
  console.log('Integrated gameplay replay: paused start, queued movement, boulder push, live #GIVE/#IDLE object, and six deterministic cycles produce a complete production-state snapshot');
})().catch(error => { console.error(error); process.exitCode = 99; });
