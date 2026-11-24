program BOARDRLE;
uses Crt, Video, GameVars, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
type
	TBoardSnapshot = array[0..3000] of byte;
var
	output: file;
	fixture: array[0..255] of byte;
	snapshot: TBoardSnapshot;
	runByte: integer;

procedure CaptureRun(count: byte);
var
	remainingCells, runCount, position, ix, iy, offset: integer;
begin
	FillChar(fixture, SizeOf(fixture), 0);
	position := SizeOf(Board.Name);
	fixture[position] := count;
	fixture[position + 1] := 7;
	fixture[position + 2] := 8;
	Inc(position, 3);
	if count = 0 then remainingCells := 1500 - 256
	else remainingCells := 1500 - count;
	while remainingCells > 0 do begin
		if remainingCells > 255 then runCount := 255
		else runCount := remainingCells;
		fixture[position] := runCount;
		fixture[position + 1] := 1;
		fixture[position + 2] := 2;
		Inc(position, 3);
		remainingCells := remainingCells - runCount;
	end;
	fixture[position] := 77;
	Inc(position, SizeOf(Board.Info));
	fixture[position] := 0;
	fixture[position + 1] := 0;
	Inc(position, 2 + SizeOf(Board.Stats[0]));

	World.BoardCount := 0;
	World.Info.CurrentBoard := 0;
	World.BoardData[0] := @fixture;
	World.BoardLen[0] := position;
	BoardOpen(0);

	offset := 0;
	for iy := 1 to 25 do
		for ix := 1 to 60 do begin
			snapshot[offset] := Ord(Board.Tiles[ix][iy].Element);
			snapshot[offset + 1] := Board.Tiles[ix][iy].Color;
			Inc(offset, 2);
		end;
	snapshot[3000] := Board.Info.MaxShots;
	BlockWrite(output, snapshot, SizeOf(snapshot));
end;

begin
	New(IoTmpBuf);
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	WorldCreate;
	Assign(output, 'BOARDRLE.BIN');
	Rewrite(output, 1);
	for runByte := 0 to 255 do
		CaptureRun(runByte);
	Close(output);
end.
