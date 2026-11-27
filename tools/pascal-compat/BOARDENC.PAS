program BOARDENC;
uses Crt, Video, GameVars, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
	brdFile: file;
	encoded: array[0..68] of byte;
	signedValues: array[0..4] of integer;
	signedNames: array[0..4] of string[8];
	ix, iy, i, tag, boardLength: integer;
begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	New(IoTmpBuf);
	Assign(output, 'BOARDENC.OUT');
	Rewrite(output);
	WorldCreate;
	if not WorldLoad('DEMO', '.ZZT', false) then Halt(1);
	{ Capture-only flush; it does not alter loaded game state or probe output. }
	Flush(output);
	for ix := 1 to BOARD_WIDTH do
		for iy := 1 to BOARD_HEIGHT do begin
			Board.Tiles[ix][iy].Element := E_EMPTY;
			Board.Tiles[ix][iy].Color := $70;
		end;
	BoardClose;
	if World.BoardLen[World.Info.CurrentBoard] < SizeOf(encoded) then Halt(2);
	Move(World.BoardData[World.Info.CurrentBoard]^, encoded, SizeOf(encoded));
	boardLength := World.BoardLen[World.Info.CurrentBoard];
	Assign(brdFile, 'BOARDENC.BRD');
	Rewrite(brdFile, 1);
	BlockWrite(brdFile, boardLength, SizeOf(word));
	BlockWrite(brdFile, World.BoardData[World.Info.CurrentBoard]^, boardLength);
	Close(brdFile);
	WorldSave('BOARDENC', '.ZZT');

	Write(output, 'runs');
	for i := 0 to 5 do
		Write(output, ' ', encoded[SizeOf(Board.Name) + (i * SizeOf(TRleTile))]);
	WriteLn(output);

	Write(output, 'tiles');
	for i := 0 to 5 do begin
		Write(output, ' ', encoded[SizeOf(Board.Name) + (i * SizeOf(TRleTile)) + 1]);
		Write(output, ' ', encoded[SizeOf(Board.Name) + (i * SizeOf(TRleTile)) + 2]);
	end;
	WriteLn(output);

	{ Emit real BoardClose records covering every signed word field at the
	  Pascal Integer boundary values. DataLen stays untouched so the source
	  board's script allocation and alias semantics remain valid. }
	signedValues[0] := -32768;
	signedValues[1] := -1;
	signedValues[2] := 0;
	signedValues[3] := 1;
	signedValues[4] := 32767;
	signedNames[0] := 'SIG0.BRD';
	signedNames[1] := 'SIG1.BRD';
	signedNames[2] := 'SIG2.BRD';
	signedNames[3] := 'SIG3.BRD';
	signedNames[4] := 'SIG4.BRD';
	for tag := 0 to 4 do begin
		Board.Info.TimeLimitSec := signedValues[tag];
		for ix := 0 to Board.StatCount do
			with Board.Stats[ix] do begin
				StepX := signedValues[tag];
				StepY := signedValues[tag];
				Cycle := signedValues[tag];
				Follower := signedValues[tag];
				Leader := signedValues[tag];
				DataPos := signedValues[tag];
			end;
		BoardClose;
		boardLength := World.BoardLen[World.Info.CurrentBoard];
		Assign(brdFile, signedNames[tag]);
		Rewrite(brdFile, 1);
		BlockWrite(brdFile, boardLength, SizeOf(word));
		BlockWrite(brdFile, World.BoardData[World.Info.CurrentBoard]^, boardLength);
		Close(brdFile);
		WriteLn(output, 'signed ', tag, ' ', signedValues[tag]);
	end;

	Close(output);
end.
