program EditorFloodFillDenseQueueProbe;
uses Crt, Video, GameVars, Game, Input, Editor;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
	x, y, filled, emptyTiles: integer;
begin
	VideoWriteText := QuietWrite;
	New(IoTmpBuf);
	WorldCreate;
	BoardCreate;
	World.BoardCount := 1;
	World.Info.CurrentBoard := 0;
	for y := 1 to BOARD_HEIGHT do
		for x := 1 to BOARD_WIDTH do begin
			Board.Tiles[x][y].Element := E_SOLID;
			Board.Tiles[x][y].Color := $0F;
		end;
	Board.Stats[0].X := 1;
	Board.Stats[0].Y := 1;
	Board.Tiles[1][1].Element := E_PLAYER;
	Board.Tiles[1][1].Color := $0F;
	for y := 4 to 20 do
		for x := 20 to 36 do
			Board.Tiles[x][y].Element := E_EMPTY;
	EditorLoop;
	filled := 0;
	emptyTiles := 0;
	for y := 4 to 20 do
		for x := 20 to 36 do begin
			if (Board.Tiles[x][y].Element = E_SOLID)
				and (Board.Tiles[x][y].Color = $0E) then filled := filled + 1;
			if Board.Tiles[x][y].Element = E_EMPTY then emptyTiles := emptyTiles + 1;
		end;
	Assign(output, 'EDFLDQ17.OUT');
	Rewrite(output);
	WriteLn(output, 'dense-queue filled ', filled, ' empty ', emptyTiles);
	for y := 3 to 21 do begin
		for x := 19 to 37 do begin
			if Board.Tiles[x][y].Element = E_EMPTY then
				Write(output, '.')
			else if Board.Tiles[x][y].Element = E_SOLID then
				if Board.Tiles[x][y].Color = $0E then
					Write(output, 'F')
				else
					Write(output, 'S')
			else if Board.Tiles[x][y].Element = E_PLAYER then
				Write(output, 'P')
			else
				Write(output, '?');
		end;
		WriteLn(output);
	end;
	Close(output);
end.
