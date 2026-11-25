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
	ix, iy, i, boardLength: integer;
begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	New(IoTmpBuf);
	Assign(output, 'BOARDENC.OUT');
	Rewrite(output);
	WorldCreate;
	if not WorldLoad('DEMO', '.ZZT', false) then Halt(1);
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
	Close(output);
end.
