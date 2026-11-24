program EditorFloodFillOverflowProbe;
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
	for y := 1 to BOARD_HEIGHT do
		if (y mod 2) = 1 then
			for x := 1 to BOARD_WIDTH do
				Board.Tiles[x][y].Element := E_EMPTY;
	for y := 2 to BOARD_HEIGHT do
		if (y mod 2) = 0 then
			if ((y div 2) mod 2) = 1 then
				Board.Tiles[BOARD_WIDTH][y].Element := E_EMPTY
			else
				Board.Tiles[1][y].Element := E_EMPTY;
	Board.Stats[0].X := 30;
	Board.Stats[0].Y := 2;
	Board.Tiles[30][2].Element := E_PLAYER;
	Board.Tiles[30][2].Color := $0F;
	EditorLoop;
	filled := 0;
	emptyTiles := 0;
	for y := 1 to BOARD_HEIGHT do
		for x := 1 to BOARD_WIDTH do begin
			if (Board.Tiles[x][y].Element = E_SOLID)
				and (Board.Tiles[x][y].Color = $0E) then filled := filled + 1;
			if Board.Tiles[x][y].Element = E_EMPTY then emptyTiles := emptyTiles + 1;
		end;
	Assign(output, 'EDFLDOVR.OUT');
	Rewrite(output);
	WriteLn(output, 'queue-wrap filled ', filled, ' empty ', emptyTiles);
	for y := 1 to BOARD_HEIGHT do begin
		for x := 1 to BOARD_WIDTH do begin
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
