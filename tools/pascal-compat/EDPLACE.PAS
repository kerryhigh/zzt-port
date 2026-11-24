program EditorPlacementProbe;
uses Crt, Video, GameVars, Game, Input, Editor;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
begin
	VideoWriteText := QuietWrite;
	New(IoTmpBuf);
	WorldCreate;
	BoardCreate;
	World.BoardCount := 1;
	World.Info.CurrentBoard := 0;
	Board.StatCount := 0;
	Board.Stats[0].X := 31;
	Board.Stats[0].Y := 12;
	Board.Tiles[30][12].Element := E_SOLID;
	Board.Tiles[30][12].Color := $0F;
	Board.Tiles[31][12].Element := E_PLAYER;
	Board.Tiles[31][12].Color := $0F;
	Board.Tiles[32][12].Element := E_SOLID;
	Board.Tiles[32][12].Color := $0F;
	EditorLoop;
	Assign(output, 'EDPLACE.OUT');
	Rewrite(output);
	WriteLn(output, 'N ', Board.Tiles[30][12].Element, ' ',
		Board.Tiles[30][12].Color, ' ', Board.Stats[0].P1);
	WriteLn(output, 'P ', Board.Tiles[31][12].Element, ' ',
		Board.Tiles[31][12].Color, ' ', Board.Stats[0].P2);
	WriteLn(output, 'C ', Board.Tiles[32][12].Element, ' ',
		Board.Tiles[32][12].Color, ' ', Board.Stats[0].P3);
	WriteLn(output, 'L ', Board.Tiles[34][12].Element, ' ',
		Board.Tiles[34][12].Color, ' ', Board.Stats[0].DataPos, ' ',
		Board.StatCount);
	Close(output);
end.
