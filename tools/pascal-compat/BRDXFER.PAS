{$M 49152,163840,655360}
program EditorBoardTransferProbe;
uses Crt, Video, GameVars, Game, Input, Elements, Editor;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
	outputName: string;
begin
	VideoWriteText := QuietWrite;
	New(IoTmpBuf);
	WorldCreate;
	World.BoardCount := 1;
	World.Info.CurrentBoard := 0;
	World.Info.Name := ParamStr(1);
	Board.Name := 'BEFORE';
	Board.Info.MaxShots := 41;
	Board.Info.NeighborBoards[0] := 1;
	Board.Info.NeighborBoards[1] := 2;
	Board.Info.NeighborBoards[2] := 3;
	Board.Info.NeighborBoards[3] := 4;
	Board.Tiles[30][12].Element := E_SOLID;
	Board.Tiles[30][12].Color := $2F;
	if (World.Info.Name = 'SHORTHD') or (World.Info.Name = 'EMPTY') then
		GetMem(World.BoardData[1], 60000);
	GetMem(World.BoardData[0], 1);
	World.BoardLen[0] := 1;
	BoardClose;

	InputKeyPressed := #0;
	EditorLoop;

	outputName := ParamStr(2);
	Assign(output, outputName);
	Rewrite(output);
	WriteLn(output, 'board ', Length(Board.Name), ' ', Board.Info.MaxShots, ' ',
		Board.StatCount, ' ', Board.Tiles[30][12].Element, ' ',
		Board.Tiles[30][12].Color, ' ', Board.Stats[0].X, ' ',
		Board.Stats[0].Y, ' ', World.BoardLen[0], ' ',
		Board.Info.NeighborBoards[0], ' ', Board.Info.NeighborBoards[1], ' ',
		Board.Info.NeighborBoards[2], ' ', Board.Info.NeighborBoards[3]);
	Close(output);
end.
