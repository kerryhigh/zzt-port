const fs = require('node:fs');
const path = require('node:path');
const shortLengthOption = process.argv.indexOf('--pascal-short-length');
const pascalShortLengthOutputPath = shortLengthOption >= 0 ? process.argv[shortLengthOption + 1] : null;
const pascalShortLengthFixturePath = shortLengthOption >= 0 ? process.argv[shortLengthOption + 2] : null;
const shortPayloadOption = process.argv.indexOf('--pascal-short-payload');
const pascalShortPayloadOutputPath = shortPayloadOption >= 0 ? process.argv[shortPayloadOption + 1] : null;
const pascalShortPayloadFixturePath = shortPayloadOption >= 0 ? process.argv[shortPayloadOption + 2] : null;
const missingTrailingOption = process.argv.indexOf('--pascal-missing-trailing');
const pascalMissingTrailingOutputPath = missingTrailingOption >= 0 ? process.argv[missingTrailingOption + 1] : null;
const pascalMissingTrailingFixturePath = missingTrailingOption >= 0 ? process.argv[missingTrailingOption + 2] : null;
if (shortLengthOption >= 0 && (!pascalShortLengthOutputPath || !pascalShortLengthFixturePath)) {
  throw new Error('Usage: run.js <world_roundtrip.wasm> [--pascal-short-length SHORTLEN.OUT PARTLEN.ZZT] [--pascal-short-payload SHORTRD.OUT PARTIAL.ZZT]');
}
if (shortPayloadOption >= 0 && (!pascalShortPayloadOutputPath || !pascalShortPayloadFixturePath)) {
  throw new Error('Usage: run.js <world_roundtrip.wasm> [--pascal-short-length SHORTLEN.OUT PARTLEN.ZZT] [--pascal-short-payload SHORTRD.OUT PARTIAL.ZZT] [--pascal-missing-trailing FULLLOAD.OUT FULLFILE.ZZT]');
}
if (missingTrailingOption >= 0 && (!pascalMissingTrailingOutputPath || !pascalMissingTrailingFixturePath)) {
  throw new Error('Usage: run.js <world_roundtrip.wasm> [--pascal-short-length SHORTLEN.OUT PARTLEN.ZZT] [--pascal-short-payload SHORTRD.OUT PARTIAL.ZZT] [--pascal-missing-trailing FULLLOAD.OUT FULLFILE.ZZT]');
}
function parseBoard(bytes, start) {
  let p = start + 51, cells = 0;
  while (cells < 1500) { const count = bytes[p]; p += 3; cells += count === 0 ? 1 : count; }
  const infoStart = p; p += 86;
  const statCount = bytes.readInt16LE(p); p += 2;
  const stats = [];
  for (let i = 0; i <= statCount; i++) { const at = p; const dataLen = bytes.readInt16LE(at + 23); p += 33; if (dataLen > 0) p += dataLen; stats.push({ at, dataLen }); }
  return { infoStart, statCount, stats, end: p };
}
(async () => {
  const original = fs.readFileSync(path.resolve(__dirname, '..', '..', 'app', 'src', 'main', 'assets', 'DEMO.ZZT'));
  const module = await WebAssembly.compile(fs.readFileSync(path.resolve(process.argv[2] || path.join(__dirname, 'world_roundtrip.wasm'))));
  const instance = await WebAssembly.instantiate(module, {});
  const { memory, input_ptr, output_ptr, output_len, run_roundtrip, run_signed_serialization_codec, run_game_save_roundtrip, run_world_read, run_world_read_preserves_tail, run_world_read_title_only, run_world_load_missing_trailing, run_world_load_partial_length, run_world_load_short_payload, run_world_load_unsigned_board_length, run_flag_save_roundtrip, run_repeated_pstring_assignment_roundtrip, run_pstring_matches_pascal_checks, run_world_reset_string_roundtrip,
    run_board_roundtrip, run_zero_count_rle, run_final_rle_overrun, run_board_rle_writer_boundary,
    run_editor_brd_roundtrip, run_editor_brd_record_roundtrip, run_editor_brd_zero_count_rle,
    run_editor_brd_reject_truncated, run_shared_stat_data_alias } = instance.exports;
  function roundTrip(input, expected, label, log = true) {
    new Uint8Array(memory.buffer, input_ptr(), input.length).set(input);
    const rc = run_roundtrip(input.length);
    if (rc !== 0) throw new Error(`${label}: serializer returned ${rc}`);
    const output = Buffer.from(new Uint8Array(memory.buffer, output_ptr(), output_len()).slice());
    if (!output.equals(expected)) {
      let at = 0; while (at < Math.min(output.length, expected.length) && output[at] === expected[at]) at++;
      throw new Error(`${label}: mismatch at ${at}; input=${input.length} expected=${expected.length} output=${output.length}`);
    }
    if (log) console.log(`${label}: ${output.length} bytes match`);
  }
  const signedCodecResult = run_signed_serialization_codec();
  if (signedCodecResult !== 0) throw new Error(`Signed Pascal serialization codec returned ${signedCodecResult}`);
  console.log('Signed Pascal serialization: all 65,536 little-endian Short bit patterns and 256 packed signed-byte values preserve their DOS interpretation');
  function externalBoards(input, label) {
    new Uint8Array(memory.buffer, input_ptr(), input.length).set(input);
    const count = input.readInt16LE(2); let pos = 512, tested = 0;
    for (let i = 0; i <= count; i++) {
      const len = input.readInt16LE(pos), start = pos + 2;
      const rc = run_board_roundtrip(start, len);
      if (rc !== 0) throw new Error(`${label} board ${i}: serializer returned ${rc}`);
      const output = Buffer.from(new Uint8Array(memory.buffer, output_ptr(), output_len()).slice());
      const expected = input.subarray(start, start + len);
      if (!output.equals(expected)) { let at=0; while(at<Math.min(output.length,expected.length)&&output[at]===expected[at])at++; throw new Error(`${label} board ${i}: mismatch at board offset ${at}; sizes ${output.length}/${expected.length}`); }
      tested++; pos = start + len;
    }
    console.log(`${label}: ${tested} external board records byte-identical`);
  }
  function editorBoardRoundTrips(input, label) {
    new Uint8Array(memory.buffer, input_ptr(), input.length).set(input);
    const count = input.readInt16LE(2); let position = 512, tested = 0;
    for (let i = 0; i <= count; i++) {
      const length = input.readInt16LE(position), start = position + 2;
      const rc = run_editor_brd_roundtrip(start, length);
      if (rc !== 0) throw new Error(`${label} board ${i}: editor parser/serializer returned ${rc}`);
      const output = Buffer.from(new Uint8Array(memory.buffer, output_ptr(), output_len()).slice());
      const expected = input.subarray(start, start + length);
      if (!output.equals(expected)) {
        let at = 0; while (at < Math.min(output.length, expected.length) && output[at] === expected[at]) at++;
        throw new Error(`${label} board ${i}: editor .BRD round-trip mismatch at offset ${at}; sizes ${output.length}/${expected.length}`);
      }
      tested++; position = start + length;
    }
    console.log(`${label}: shared .BRD parser/exporter round-trip ${tested} board records byte-for-byte`);
  }
  let pos = 512;
  const currentBoard = original.readInt16LE(17);
  let originalCurrentBoardStart = -1;
  let originalCurrentBoard = null;
  const flagOffset = 50;
  const flagStride = 21;
  for (let i = 0; i <= currentBoard; i++) { const len = original.readInt16LE(pos); if (i === currentBoard) { originalCurrentBoardStart = pos + 2; originalCurrentBoard = parseBoard(original, originalCurrentBoardStart); const s = originalCurrentBoard.stats[0]; if (s.dataLen !== 0 || original.readInt16LE(s.at + 21) === 0) throw new Error('Fixture no longer contains nonzero DataPos with zero DataLen'); } pos += 2 + len; }
  roundTrip(original, original, 'Official DEMO.ZZT current-board save');
  if (original[264] !== 0) throw new Error(`DEMO.ZZT fixture IsSave byte changed unexpectedly: ${original[264]}`);
  new Uint8Array(memory.buffer, input_ptr(), original.length).set(original);
  const gameSaveResult = run_game_save_roundtrip(original.length);
  if (gameSaveResult !== 0) throw new Error(`Production zoo_world_save returned ${gameSaveResult}`);
  const gameSave = Buffer.from(new Uint8Array(memory.buffer, output_ptr(), output_len()).slice());
  const expectedGameSave = Buffer.from(original);
  expectedGameSave[264] = 1;
  if (!gameSave.equals(expectedGameSave)) {
    let at = 0; while (at < Math.min(gameSave.length, expectedGameSave.length) && gameSave[at] === expectedGameSave[at]) at++;
    throw new Error(`Production .SAV save mismatch at ${at}; expected only the Pascal IsSave byte to change, sizes ${gameSave.length}/${expectedGameSave.length}`);
  }
  console.log(`Production zoo_world_save .SAV output: ${gameSave.length} bytes; only IsSave byte 264 changes from 0 to 1`);
  roundTrip(gameSave, gameSave, 'Production .SAV load and resave');

  // A Black key is the unchecked Pascal Keys[0] alias into Gems' high byte.
  // Keep the seven real Keys[1..7] bytes clear and prove that alias survives
  // both ordinary world serialization and the production game-save path.
  const blackKeyWorld = Buffer.from(original);
  blackKeyWorld.writeInt16LE(0x01FA, 6);
  blackKeyWorld.fill(0, 8, 15);
  roundTrip(blackKeyWorld, blackKeyWorld, 'Black-key Gems alias .ZZT round-trip');
  new Uint8Array(memory.buffer, input_ptr(), blackKeyWorld.length).set(blackKeyWorld);
  const blackKeySaveResult = run_game_save_roundtrip(blackKeyWorld.length);
  if (blackKeySaveResult !== 0) throw new Error(`Black-key Gems alias .SAV save returned ${blackKeySaveResult}`);
  const blackKeySave = Buffer.from(new Uint8Array(memory.buffer, output_ptr(), output_len()).slice());
  const expectedBlackKeySave = Buffer.from(blackKeyWorld);
  expectedBlackKeySave[264] = 1;
  if (!blackKeySave.equals(expectedBlackKeySave)) {
    let at = 0; while (at < Math.min(blackKeySave.length, expectedBlackKeySave.length)
        && blackKeySave[at] === expectedBlackKeySave[at]) at++;
    throw new Error(`Black-key Gems alias .SAV differs from Pascal record bytes at ${at}`);
  }
  roundTrip(blackKeySave, blackKeySave, 'Black-key Gems alias .SAV load and resave');
  console.log('Black key file state: Gems high-byte alias and clear Keys[1..7] bytes survive .ZZT, production .SAV save, and resave');

  function worldRead(input, label, shouldSucceed) {
    new Uint8Array(memory.buffer, input_ptr(), input.length).set(input);
    const rc = run_world_read(input.length);
    if (shouldSucceed ? rc !== 0 : rc === 0) {
      throw new Error(`${label}: unexpected world-read result ${rc}`);
    }
    console.log(`${label}: ${shouldSucceed ? 'accepted' : `rejected (${rc})`}`);
  }
  const firstBoardLength = original.readInt16LE(512);
  worldRead(original.subarray(0, 511), 'World header truncated by one byte', false);
  worldRead(original.subarray(0, 513), 'Board-length word truncated by one byte', true);
  const partialBoard = original.subarray(0, 514 + firstBoardLength - 1);
  worldRead(partialBoard, 'Declared first-board payload truncated by one byte', true);
  new Uint8Array(memory.buffer, input_ptr(), partialBoard.length).set(partialBoard);
  const tailResult = run_world_read_preserves_tail(partialBoard.length, firstBoardLength);
  if (tailResult !== 0) throw new Error(`Partial board read did not preserve the prefilled destination tail (${tailResult})`);
  console.log('Unchecked Pascal board reads: partial length words retain unread destination bytes, and short payload tails remain untouched in the allocated buffer');
  const titleOnlyPrefix = original.subarray(0, 514 + firstBoardLength);
  new Uint8Array(memory.buffer, input_ptr(), titleOnlyPrefix.length).set(titleOnlyPrefix);
  const titleOnlyResult = run_world_read_title_only(titleOnlyPrefix.length);
  if (titleOnlyResult !== 0) throw new Error(`Pascal title-only world load returned ${titleOnlyResult}`);
  console.log('Pascal title-only load: first board opens with a header that declares more, unprovided boards');
  worldRead(titleOnlyPrefix, 'Full-world read with missing trailing board records', true);
  const fullLoadFixture = Buffer.from(titleOnlyPrefix);
  fullLoadFixture.writeInt16LE(0, 17);
  new Uint8Array(memory.buffer, input_ptr(), fullLoadFixture.length).set(fullLoadFixture);
  const fullLoadResult = run_world_load_missing_trailing(fullLoadFixture.length);
  if (fullLoadResult !== 0) throw new Error(`Full-world load of a valid current board with missing trailing records returned ${fullLoadResult}`);
  const fullLoadSnapshot = Buffer.from(new Uint8Array(memory.buffer, output_ptr(), output_len()).slice());
  if (fullLoadSnapshot.length !== 13) throw new Error(`Production missing-trailing snapshot has ${fullLoadSnapshot.length} bytes; expected 13.`);
  const fullLoadProductionFields = [
    fullLoadSnapshot[0],
    fullLoadSnapshot.readInt16LE(1),
    fullLoadSnapshot.readInt16LE(3),
    fullLoadSnapshot[5],
    fullLoadSnapshot[6],
    fullLoadSnapshot.readInt16LE(7),
    fullLoadSnapshot.readInt16LE(9),
    fullLoadSnapshot[11],
    fullLoadSnapshot[12]
  ];
  const expectedFullLoadFields = [1, 15, 0, 0, 1, 0, 0];
  if (fullLoadProductionFields.slice(0, expectedFullLoadFields.length).some((value, i) => value !== expectedFullLoadFields[i])) {
    throw new Error(`Production missing-trailing state ${fullLoadProductionFields.join(' ')} differed from expected ${expectedFullLoadFields.join(' ')}.`);
  }
  if (pascalMissingTrailingOutputPath) {
    const missingTrailingFixture = fs.readFileSync(path.resolve(pascalMissingTrailingFixturePath));
    if (!missingTrailingFixture.equals(fullLoadFixture)) {
      throw new Error('Pascal FULLFILE.ZZT does not match the expected missing-trailing-board fixture.');
    }
    const pascalText = fs.readFileSync(path.resolve(pascalMissingTrailingOutputPath), 'utf8').trim();
    const match = /^full-load (\d+) (\d+) (\d+) (\d+) (\d+) missing-len (\d+) (\d+) data-present (\d+) (\d+)$/.exec(pascalText);
    if (!match) throw new Error(`Unexpected Pascal FULLLOAD.OUT record: ${pascalText}`);
    const pascalFields = match.slice(1).map(Number);
    for (let i = 0; i < expectedFullLoadFields.length; i++) {
      if (pascalFields[i] !== fullLoadProductionFields[i]) {
        throw new Error(`Missing-trailing WorldLoad differential mismatch at field ${i}: Pascal=${pascalFields[i]}, production=${fullLoadProductionFields[i]}; Pascal=${pascalText}; production=${fullLoadProductionFields.join(' ')}`);
      }
    }
    const pascalZeroLengthPresence = pascalFields.slice(7);
    const productionZeroLengthPresence = fullLoadProductionFields.slice(7);
    console.log(`Direct Pascal/production .ZZT WorldLoad with absent trailing board records matched seven deterministic fields: ${fullLoadProductionFields.slice(0, 7).join(' ')}. Zero-length BoardData[1]/[15] presence was Pascal=${pascalZeroLengthPresence.join('/')}, production=${productionZeroLengthPresence.join('/')}; this is reported separately under the allocation nondeterminism contract, and downstream board-open effects remain unverified.`);
  } else {
    console.log('Production full-world load opens the valid current board and retains the declared count when trailing board records are absent');
  }
  const expectedPartialLengthFixture = Buffer.concat([fullLoadFixture, Buffer.from([0x10])]);
  const partialLengthFixture = pascalShortLengthFixturePath
    ? fs.readFileSync(path.resolve(pascalShortLengthFixturePath))
    : expectedPartialLengthFixture;
  if (!partialLengthFixture.equals(expectedPartialLengthFixture)) {
    throw new Error('Pascal PARTLEN.ZZT does not match the expected one-byte partial board-length fixture.');
  }
  new Uint8Array(memory.buffer, input_ptr(), partialLengthFixture.length).set(partialLengthFixture);
  const partialLengthResult = run_world_load_partial_length(partialLengthFixture.length);
  if (partialLengthResult !== 0) throw new Error(`A one-byte board length did not preserve its prior high byte (${partialLengthResult})`);
  const partialLengthProduction = Buffer.from(new Uint8Array(memory.buffer, output_ptr(), output_len()).slice());
  if (partialLengthProduction.length !== 9) throw new Error(`Production partial-length snapshot has ${partialLengthProduction.length} bytes; expected 9.`);
  const productionPartialLengthFields = [
    partialLengthProduction[0],
    partialLengthProduction.readInt16LE(1),
    partialLengthProduction.readInt16LE(3),
    partialLengthProduction[5],
    partialLengthProduction.readInt16LE(6),
    partialLengthProduction[8]
  ];
  if (pascalShortLengthOutputPath) {
    const pascalText = fs.readFileSync(path.resolve(pascalShortLengthOutputPath), 'utf8').trim();
    const match = /^short-len (\d+) (\d+) (\d+) (\d+) (\d+) (\d+)$/.exec(pascalText);
    if (!match) throw new Error(`Unexpected Pascal SHORTLEN.OUT record: ${pascalText}`);
    const pascalPartialLengthFields = match.slice(1).map(Number);
    for (let i = 0; i < pascalPartialLengthFields.length; i++) {
      if (pascalPartialLengthFields[i] !== productionPartialLengthFields[i]) {
        throw new Error(`Partial WorldLoad length differential mismatch at field ${i}: Pascal=${pascalPartialLengthFields[i]}, production=${productionPartialLengthFields[i]}; Pascal=${pascalText}; production=${productionPartialLengthFields.join(' ')}`);
      }
    }
    console.log(`Direct Pascal/production one-byte partial .ZZT board-length load matched six fields: ${productionPartialLengthFields.join(' ')} (the prior high byte remains ${productionPartialLengthFields[4] >> 8}).`);
  } else {
    const expectedPartialLengthFields = [1, 15, 0, 0, 0x0110, 1];
    if (productionPartialLengthFields.some((value, i) => value !== expectedPartialLengthFields[i])) {
      throw new Error(`Production partial-length state ${productionPartialLengthFields.join(' ')} differed from expected ${expectedPartialLengthFields.join(' ')}.`);
    }
    console.log('Production full-world load preserves the prior high byte of a one-byte board-length read');
  }
  if (pascalShortPayloadOutputPath) {
    const shortPayloadFixture = fs.readFileSync(path.resolve(pascalShortPayloadFixturePath));
    const expectedShortPayloadFixture = Buffer.concat([
      fullLoadFixture,
      Buffer.from([16, 0, 0x11, 0x22, 0x33, 0x44])
    ]);
    if (!shortPayloadFixture.equals(expectedShortPayloadFixture)) {
      throw new Error('Pascal PARTIAL.ZZT does not match the expected four-byte short-payload fixture.');
    }
    new Uint8Array(memory.buffer, input_ptr(), shortPayloadFixture.length).set(shortPayloadFixture);
    const shortPayloadResult = run_world_load_short_payload(shortPayloadFixture.length);
    if (shortPayloadResult !== 0) throw new Error(`Production full-world short-payload load returned ${shortPayloadResult}`);
    const productionSnapshot = Buffer.from(new Uint8Array(memory.buffer, output_ptr(), output_len()).slice());
    if (productionSnapshot.length !== 13) throw new Error(`Production short-payload snapshot has ${productionSnapshot.length} bytes; expected 13.`);
    const productionFields = [
      productionSnapshot[0],
      productionSnapshot.readInt16LE(1),
      productionSnapshot.readInt16LE(3),
      productionSnapshot[5],
      productionSnapshot.readInt16LE(6),
      productionSnapshot[8],
      ...productionSnapshot.subarray(9, 13)
    ];
    const pascalText = fs.readFileSync(path.resolve(pascalShortPayloadOutputPath), 'utf8').trim();
    const match = /^short-read (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) data (\d+) (\d+) (\d+) (\d+) tail (\d+)$/.exec(pascalText);
    if (!match) throw new Error(`Unexpected Pascal SHORTRD.OUT record: ${pascalText}`);
    const pascalFields = match.slice(1, 11).map(Number);
    for (let i = 0; i < pascalFields.length; i++) {
      if (pascalFields[i] !== productionFields[i]) {
        throw new Error(`Short-payload WorldLoad differential mismatch at field ${i}: Pascal=${pascalFields[i]}, production=${productionFields[i]}; Pascal=${pascalText}; production=${productionFields.join(' ')}`);
      }
    }
    const tailByte = Number(match[11]);
    if (tailByte > 255) throw new Error(`Pascal short-payload tail was outside byte range: ${tailByte}`);
    console.log(`Direct Pascal/production short-payload .ZZT WorldLoad matched ten deterministic fields: ${productionFields.join(' ')}; unread tail byte ${tailByte} is allocator-dependent and excluded.`);
  }
  const unsignedLengthFixture = Buffer.alloc(514 + 0xffff);
  original.copy(unsignedLengthFixture, 0, 0, 514 + firstBoardLength);
  unsignedLengthFixture.writeInt16LE(0, 2);
  unsignedLengthFixture.writeInt16LE(0, 17);
  unsignedLengthFixture.writeInt16LE(-1, 512);
  new Uint8Array(memory.buffer, input_ptr(), unsignedLengthFixture.length).set(unsignedLengthFixture);
  const unsignedLengthResult = run_world_load_unsigned_board_length(unsignedLengthFixture.length);
  if (unsignedLengthResult !== 0) throw new Error(`A -1 board length did not load as a 65535-byte Pascal Word (${unsignedLengthResult})`);
  console.log('Production full-world load accepts board length -1 as an unsigned 65535-byte record and opens board 0');
  worldRead(Buffer.concat([original, Buffer.from([0x5a, 0x5a, 0x54])]), 'World trailing bytes after declared boards', true);
  externalBoards(original, 'Official DEMO.ZZT');
  editorBoardRoundTrips(original, 'Official DEMO.ZZT');
  const unsignedBrdFixture = Buffer.alloc(2 + 0xffff);
  unsignedBrdFixture.writeUInt16LE(0xffff, 0);
  original.copy(unsignedBrdFixture, 2, 514, 514 + firstBoardLength);
  new Uint8Array(memory.buffer, input_ptr(), unsignedBrdFixture.length).set(unsignedBrdFixture);
  const unsignedBrdResult = run_editor_brd_record_roundtrip(unsignedBrdFixture.length);
  if (unsignedBrdResult !== 0) throw new Error(`Production .BRD import rejected an unsigned 65535-byte record (${unsignedBrdResult})`);
  const unsignedBrdOutput = Buffer.from(new Uint8Array(memory.buffer, output_ptr(), output_len()).slice());
  const expectedUnsignedBrdOutput = original.subarray(514, 514 + firstBoardLength);
  if (!unsignedBrdOutput.equals(expectedUnsignedBrdOutput)) {
    let at = 0; while (at < Math.min(unsignedBrdOutput.length, expectedUnsignedBrdOutput.length)
        && unsignedBrdOutput[at] === expectedUnsignedBrdOutput[at]) at++;
    throw new Error(`Production .BRD import with a 0xFFFF length changed the parsed board at ${at}`);
  }
  console.log('Production .BRD import treats 0xFFFF as a 65535-byte Word count and ignores trailing payload padding');
  const truncatedBrdResult = run_editor_brd_reject_truncated();
  if (truncatedBrdResult !== 0) throw new Error(`Production .BRD truncation rejection differed from Pascal BlockRead errors (${truncatedBrdResult})`);
  console.log('Production .BRD parser rejects empty/partial length words and declared payloads truncated at 0 through 3 bytes');
  const pstringMatchResult = run_pstring_matches_pascal_checks();
  if (pstringMatchResult !== 0) throw new Error(`Pascal short-string unchanged-value check returned ${pstringMatchResult}`);
  console.log('Pascal short-string editor comparison: embedded NULs and overlong raw lengths remain unchanged; real edits are detected');
  const legacy = Buffer.concat([original.subarray(2, 512), Buffer.alloc(2), original.subarray(512)]);
  roundTrip(legacy, original, 'Legacy header load then canonical save');
  const mutated = Buffer.from(original);
  mutated[29] = 4; mutated.set([68, 0, 77, 79], 30);
  mutated[8] = 0x7f; mutated[264] = 0xa5;
  for (let i = 0; i < 14; i++) mutated[265 + i] = (0x91 + i) & 0xff;
  pos = 512;
  for (let i = 0; i <= currentBoard; i++) { const len = mutated.readInt16LE(pos); if (i === currentBoard) { const start=pos+2, parsed=parseBoard(mutated,start); mutated[start]=4; mutated.set([82,0,87,68],start+1); mutated[parsed.infoStart+1]=0x42; mutated[parsed.infoStart+6]=0xf3; mutated[parsed.infoStart+7]=4; mutated.set([65,0,66,67],parsed.infoStart+8); mutated[parsed.infoStart+70]=0x88; mutated[parsed.infoStart+85]=0xfe; const stat0=parsed.stats[0].at; mutated[stat0+21]=0x34; mutated[stat0+22]=0x12; for(let j=0;j<8;j++)mutated[stat0+25+j]=0xc0+j; } pos+=2+len; }
  roundTrip(mutated, mutated, 'Embedded-NUL strings, raw Boolean bytes, reserved bytes and stale DataPos');
  externalBoards(mutated, 'Mutated DEMO.ZZT');

  function fillDeterministic(bytes, start, length, seed) {
    for (let i = 0; i < length; i++) {
      seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0;
      bytes[start + i] = seed >>> 24;
    }
  }
  for (let i = 0; i < 256; i++) {
    const fixture = Buffer.from(original);
    for (let key = 0; key < 7; key++) fixture[8 + key] = (i + key * 37) & 0xff;
    fixture[264] = i;
    fillDeterministic(fixture, 25, 2, 0x8f33 + i);
    fillDeterministic(fixture, 265, 14, 0x32e7 + i * 5);
    fixture[29] = i;
    fillDeterministic(fixture, 30, 20, 0x5a17 + i);
    for (let flag = 0; flag < 10; flag++) {
      const length = (i + flag * 29) & 0xff;
      setRawFlag(fixture, flag, length, Buffer.alloc(20), 0);
      fillDeterministic(fixture, flagOffset + flag * flagStride + 1, 20,
        0x7731 + i * 17 + flag);
    }
    fixture[originalCurrentBoardStart] = i;
    fillDeterministic(fixture, originalCurrentBoardStart + 1, 50, 0x91ab + i * 31);
    fixture[originalCurrentBoard.infoStart + 1] = i;
    fixture[originalCurrentBoard.infoStart + 6] = (255 - i) & 0xff;
    fixture[originalCurrentBoard.infoStart + 7] = (255 - i) & 0xff;
    fillDeterministic(fixture, originalCurrentBoard.infoStart + 8, 58, 0x24c1 + i * 13);
    fillDeterministic(fixture, originalCurrentBoard.infoStart + 70, 16, 0x62d1 + i * 7);
    roundTrip(fixture, fixture, `Raw Pascal short-string length case ${i}`, false);
    new Uint8Array(memory.buffer, input_ptr(), fixture.length).set(fixture);
    const boardLength = fixture.readInt16LE(originalCurrentBoardStart - 2);
    const brdResult = run_editor_brd_roundtrip(originalCurrentBoardStart, boardLength);
    if (brdResult !== 0) throw new Error(`Raw .BRD field case ${i}: editor parser/serializer returned ${brdResult}`);
    const brdOutput = Buffer.from(new Uint8Array(memory.buffer, output_ptr(), output_len()).slice());
    if (!brdOutput.equals(fixture.subarray(originalCurrentBoardStart, originalCurrentBoardStart + boardLength))) {
      let at = 0; const expected = fixture.subarray(originalCurrentBoardStart, originalCurrentBoardStart + boardLength);
      while (at < Math.min(brdOutput.length, expected.length) && brdOutput[at] === expected[at]) at++;
      throw new Error(`Raw .BRD field case ${i}: mismatch at offset ${at}; expected=${expected.subarray(Math.max(0, at - 8), at + 16).toString('hex')} got=${brdOutput.subarray(Math.max(0, at - 8), at + 16).toString('hex')}; sizes ${brdOutput.length}/${expected.length}`);
    }
  }
  console.log('Pascal raw fields: 256 world and .BRD round-trips cover every short-string length byte plus every raw Boolean byte, with arbitrary CP437/NUL payloads and reserved bytes');

  function setRawFlag(fixture, flagId, length, payload, padding) {
    const at = flagOffset + flagId * flagStride;
    fixture[at] = length;
    fixture.fill(padding, at + 1, at + flagStride);
    fixture.set(payload, at + 1);
  }
  function flagSaveRoundTrip(input, action, expected, label) {
    new Uint8Array(memory.buffer, input_ptr(), input.length).set(input);
    const rc = run_flag_save_roundtrip(input.length, action);
    if (rc !== 0) throw new Error(`${label}: serializer returned ${rc}`);
    const output = Buffer.from(new Uint8Array(memory.buffer, output_ptr(), output_len()).slice());
    if (!output.equals(expected)) {
      let at = 0; while (at < Math.min(output.length, expected.length) && output[at] === expected[at]) at++;
      throw new Error(`${label}: mismatch at ${at}; input=${input.length} expected=${expected.length} output=${output.length}`);
    }
    console.log(`${label}: ${output.length} bytes match`);
  }

  const embeddedFlag = Buffer.from(original);
  for (let i = 0; i < 10; i++) setRawFlag(embeddedFlag, i, 0, Buffer.alloc(20), 0);
  setRawFlag(embeddedFlag, 0, 3, Buffer.from([65, 0, 66]), 0xa5);
  setRawFlag(embeddedFlag, 1, 0, Buffer.alloc(20), 0xd4);
  const embeddedFlagExpected = Buffer.from(embeddedFlag);
  embeddedFlagExpected[flagOffset + flagStride] = 1;
  embeddedFlagExpected[flagOffset + flagStride + 1] = 65;
  flagSaveRoundTrip(embeddedFlag, 0, embeddedFlagExpected,
    'Pascal SET A treats an embedded-NUL A\0B flag as distinct and preserves short-string tail bytes');

  const duplicateFlags = Buffer.from(original);
  for (let i = 0; i < 10; i++) setRawFlag(duplicateFlags, i, 0, Buffer.alloc(20), 0);
  setRawFlag(duplicateFlags, 0, 3, Buffer.from('DUP', 'ascii'), 0xb1);
  setRawFlag(duplicateFlags, 1, 3, Buffer.from('DUP', 'ascii'), 0xb2);
  const duplicateFlagsExpected = Buffer.from(duplicateFlags);
  duplicateFlagsExpected[flagOffset + flagStride] = 0;
  flagSaveRoundTrip(duplicateFlags, 1, duplicateFlagsExpected,
    'Pascal CLEAR DUP removes the last duplicate flag and retains its payload bytes');

  const repeatedFlags = Buffer.from(original);
  for (let i = 0; i < 10; i++) setRawFlag(repeatedFlags, i, 0, Buffer.alloc(20), 0xb5);
  const repeatedFlagsExpected = Buffer.from(repeatedFlags);
  repeatedFlagsExpected[flagOffset] = 1;
  repeatedFlagsExpected.set([65, 79, 78, 71], flagOffset + 1);
  flagSaveRoundTrip(repeatedFlags, 2, repeatedFlagsExpected,
    'Repeated Pascal flag assignment preserves the tail left by earlier longer values');

  const repeatedStrings = Buffer.from(original);
  let repeatedBoardStart = 512;
  let boardRecordStart = -1;
  let repeatedBoard = null;
  for (let i = 0; i <= currentBoard; i++) {
    const len = repeatedStrings.readInt16LE(repeatedBoardStart);
    if (i === currentBoard) {
      boardRecordStart = repeatedBoardStart + 2;
      repeatedBoard = parseBoard(repeatedStrings, boardRecordStart);
    }
    repeatedBoardStart += 2 + len;
  }
  const repeatedExpected = Buffer.from(repeatedStrings);
  repeatedExpected[29] = 2;
  repeatedExpected.set([65, 66, 78, 71], 30);
  repeatedExpected[boardRecordStart] = 2;
  repeatedExpected.set([65, 66, 78, 71], boardRecordStart + 1);
  repeatedExpected[repeatedBoard.infoStart + 7] = 0;
  repeatedExpected.set([88, 79, 78, 71], repeatedBoard.infoStart + 8);
  new Uint8Array(memory.buffer, input_ptr(), repeatedStrings.length).set(repeatedStrings);
  const repeatedResult = run_repeated_pstring_assignment_roundtrip(repeatedStrings.length);
  if (repeatedResult !== 0) throw new Error(`Repeated Pascal short-string assignment returned ${repeatedResult}`);
  const repeatedOutput = Buffer.from(new Uint8Array(memory.buffer, output_ptr(), output_len()).slice());
  if (!repeatedOutput.equals(repeatedExpected)) {
    let at = 0; while (at < Math.min(repeatedOutput.length, repeatedExpected.length) && repeatedOutput[at] === repeatedExpected[at]) at++;
    throw new Error(`Repeated Pascal world/board/message short-string assignment mismatch at ${at}; sizes ${repeatedOutput.length}/${repeatedExpected.length}`);
  }
  console.log(`Repeated Pascal world, board, and message short-string assignments retain stale payload tails (${repeatedOutput.length} bytes match)`);

  const resetStrings = Buffer.from(original);
  resetStrings[29] = 3;
  resetStrings.set([0, 66, 67], 30);
  resetStrings.fill(0xA5, 33, 50);
  for (let i = 0; i < 10; i++) setRawFlag(resetStrings, i, 0, Buffer.alloc(20), 0xC0 + i);
  setRawFlag(resetStrings, 0, 3, Buffer.from([0, 68, 69]), 0xD5);
  new Uint8Array(memory.buffer, input_ptr(), resetStrings.length).set(resetStrings);
  const resetResult = run_world_reset_string_roundtrip(resetStrings.length);
  if (resetResult !== 0) throw new Error(`WorldUnload/WorldCreate short-string reset returned ${resetResult}`);
  const resetOutput = Buffer.from(new Uint8Array(memory.buffer, output_ptr(), output_len()).slice());
  if (resetOutput[29] !== 0 || resetOutput[flagOffset] !== 0) {
    throw new Error(`WorldUnload/WorldCreate did not clear short-string lengths: name=${resetOutput[29]} flag=${resetOutput[flagOffset]}`);
  }
  if (!resetOutput.subarray(30, 50).equals(resetStrings.subarray(30, 50))
      || !resetOutput.subarray(flagOffset + 1, flagOffset + flagStride).equals(resetStrings.subarray(flagOffset + 1, flagOffset + flagStride))) {
    throw new Error('WorldUnload/WorldCreate changed fixed short-string payload bytes');
  }
  const resetBoardStart = 514;
  const resetBoard = parseBoard(resetOutput, resetBoardStart);
  const expectedTitlePayload = Buffer.from(original.subarray(originalCurrentBoardStart + 1, originalCurrentBoardStart + 51));
  expectedTitlePayload.set(Buffer.from('Title screen', 'ascii'), 0);
  if (resetOutput[resetBoardStart] !== 12
      || !resetOutput.subarray(resetBoardStart + 1, resetBoardStart + 51).equals(expectedTitlePayload)) {
    const actualTitlePayload = resetOutput.subarray(resetBoardStart + 1, resetBoardStart + 51);
    let at = 0; while (at < 50 && actualTitlePayload[at] === expectedTitlePayload[at]) at++;
    throw new Error(`WorldCreate board-name tail mismatch at ${at}: expected ${expectedTitlePayload.toString('hex')}, got ${actualTitlePayload.toString('hex')}`);
  }
  if (resetOutput[resetBoard.infoStart + 7] !== 0
      || !resetOutput.subarray(resetBoard.infoStart + 8, resetBoard.infoStart + 8 + 58)
          .equals(original.subarray(originalCurrentBoard.infoStart + 8, originalCurrentBoard.infoStart + 8 + 58))) {
    throw new Error('BoardCreate did not clear only the message length while preserving its fixed payload bytes');
  }
  console.log(`Production WorldUnload/WorldCreate resets Pascal starting-world state, clears embedded-NUL name/flag lengths, and retains fixed-string payload bytes (${resetOutput.length}-byte save)`);

  const zeroRunBoard = Buffer.alloc(51 + 6 * 3 + 86 + 2 + 33);
  zeroRunBoard[0] = 4; zeroRunBoard.set([90, 69, 82, 79], 1);
  let rle = 51; zeroRunBoard[rle++] = 0; zeroRunBoard[rle++] = 7; zeroRunBoard[rle++] = 8;
  let remaining = 1244;
  while (remaining > 0) { const count = Math.min(255, remaining); zeroRunBoard[rle++] = count; zeroRunBoard[rle++] = 1; zeroRunBoard[rle++] = 2; remaining -= count; }
  rle += 86; zeroRunBoard.writeInt16LE(0, rle);
  new Uint8Array(memory.buffer, input_ptr(), zeroRunBoard.length).set(zeroRunBoard);
  const zeroResult = run_zero_count_rle(0, zeroRunBoard.length);
  if (zeroResult !== 0) throw new Error(`Zero-count RLE byte-underflow assertion returned ${zeroResult}`);
  console.log('Zero-count RLE: one byte count underflows to 255, filling 256 cells as in Pascal');
  const finalRunBoard = Buffer.alloc(51 + 6 * 3 + 86 + 2 + 33);
  finalRunBoard[0] = 5; finalRunBoard.set([70, 73, 78, 65, 76], 1);
  let finalPos = 51;
  for (let i = 0; i < 6; i++) { finalRunBoard[finalPos++] = 255; finalRunBoard[finalPos++] = i + 1; finalRunBoard[finalPos++] = i + 11; }
  finalRunBoard[finalPos] = 77;
  finalPos += 86; finalRunBoard.writeInt16LE(0, finalPos);
  new Uint8Array(memory.buffer, input_ptr(), finalRunBoard.length).set(finalRunBoard);
  const overrunResult = run_final_rle_overrun(0, finalRunBoard.length);
  if (overrunResult !== 0) throw new Error(`Final RLE overflow assertion returned ${overrunResult}`);
  console.log('Final RLE overrun: board stops at cell 1,500 and reads info after the run record, as Pascal does');
  const writerBoundaryResult = run_board_rle_writer_boundary();
  if (writerBoundaryResult !== 0) throw new Error(`Board RLE writer boundary assertion returned ${writerBoundaryResult}`);
  console.log('Board RLE writer: 1,500 matching cells split into five 255-byte runs and one 225-byte run');
  const brdResult = run_editor_brd_zero_count_rle();
  if (brdResult !== 0) throw new Error(`External .BRD zero-count RLE assertion returned ${brdResult}`);
  console.log('External .BRD parser/exporter: zero-count runs repeat 256 cells, trailing bytes are ignored, and export writes canonical Pascal-sized runs');
  const aliasResult = run_shared_stat_data_alias();
  if (aliasResult !== 0) throw new Error(`Shared stat-data alias assertion returned ${aliasResult}`);
  const aliasBoardBytes = Buffer.from(new Uint8Array(memory.buffer, output_ptr(), output_len()).slice());
  const aliasBoard = parseBoard(aliasBoardBytes, 0);
  if (aliasBoard.stats[1].dataLen !== 8 || aliasBoard.stats[2].dataLen !== -1) {
    throw new Error(`Shared stat-data alias bytes differ from Pascal: ${aliasBoard.stats.map(stat => stat.dataLen).join(',')}`);
  }
  console.log('Shared stat data: later OOP records serialize DataLen=-1, then reload to the earlier stat’s identical pointer and payload');
})().catch(error => { console.error(error); process.exitCode = 99; });
