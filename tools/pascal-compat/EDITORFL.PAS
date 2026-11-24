program EditorFloodFillProbe;
uses Crt, Video, GameVars, Game, Input, Editor;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
	x, y, filled: integer;
begin
	VideoWriteText := QuietWrite;
	New(IoTmpBuf);
	WorldCreate;
	BoardCreate;
	World.BoardCount := 1;
	World.Info.CurrentBoard := 0;
	Board.Stats[0].X := 1;
	Board.Stats[0].Y := 1;
	for y := 1 to BOARD_HEIGHT do
		for x := 1 to BOARD_WIDTH do begin
			Board.Tiles[x][y].Element := E_SOLID;
			Board.Tiles[x][y].Color := $0F;
		end;
	for y := 11 to 13 do
		for x := 29 to 31 do begin
			Board.Tiles[x][y].Element := E_EMPTY;
			Board.Tiles[x][y].Color := $0F;
		end;
	EditorLoop;
	filled := 0;
	if (Board.Tiles[29][11].Element = E_SOLID) and (Board.Tiles[29][11].Color = $0E) then filled := filled + 1;
	if (Board.Tiles[30][11].Element = E_SOLID) and (Board.Tiles[30][11].Color = $0E) then filled := filled + 1;
	if (Board.Tiles[31][11].Element = E_SOLID) and (Board.Tiles[31][11].Color = $0E) then filled := filled + 1;
	if (Board.Tiles[29][12].Element = E_SOLID) and (Board.Tiles[29][12].Color = $0E) then filled := filled + 1;
	if (Board.Tiles[30][12].Element = E_SOLID) and (Board.Tiles[30][12].Color = $0E) then filled := filled + 1;
	if (Board.Tiles[31][12].Element = E_SOLID) and (Board.Tiles[31][12].Color = $0E) then filled := filled + 1;
	if (Board.Tiles[29][13].Element = E_SOLID) and (Board.Tiles[29][13].Color = $0E) then filled := filled + 1;
	if (Board.Tiles[30][13].Element = E_SOLID) and (Board.Tiles[30][13].Color = $0E) then filled := filled + 1;
	if (Board.Tiles[31][13].Element = E_SOLID) and (Board.Tiles[31][13].Color = $0E) then filled := filled + 1;
	Assign(output, 'EDITORFL.OUT');
	Rewrite(output);
	WriteLn(output, 'enclosed-fill ', filled, ' center ',
		Board.Tiles[30][12].Element, ' ', Board.Tiles[30][12].Color,
		' outside ', Board.Tiles[28][12].Element, ' ', Board.Tiles[28][12].Color);
	Close(output);
end.
