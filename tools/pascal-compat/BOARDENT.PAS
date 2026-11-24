program BoardEdgeTouchProbe;
uses Crt, Video, GameVars, Elements, Sounds, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
	deltaX, deltaY: integer;

procedure PrintState(labelText: string);
begin
	WriteLn(output, labelText,
		' board ', World.Info.CurrentBoard,
		' player ', Board.Stats[0].X, ' ', Board.Stats[0].Y,
		' tile ', Board.Tiles[Board.Stats[0].X][Board.Stats[0].Y].Element,
		' delta ', deltaX, ' ', deltaY,
		' start ', Board.Info.StartPlayerX, ' ', Board.Info.StartPlayerY,
		' time ', World.Info.BoardTimeSec,
		' paused ', Ord(GamePaused));
	Flush(output);
end;

procedure SetSourceBoard;
begin
	BoardCreate;
	World.Info.CurrentBoard := 0;
	Board.Stats[0].X := 30;
	Board.Stats[0].Y := 1;
	Board.Tiles[30][1].Element := E_PLAYER;
	Board.Tiles[30][1].Color := ElementDefs[E_PLAYER].Color;
	Board.Info.NeighborBoards[0] := 1;
	Board.Info.NeighborBoards[1] := 2;
	Board.Info.NeighborBoards[2] := 3;
	Board.Info.NeighborBoards[3] := 4;
	Board.Info.StartPlayerX := 30;
	Board.Info.StartPlayerY := 1;
	BoardClose;
end;

procedure SetDestinationBoard(boardId, entryX, entryY: integer);
begin
	World.Info.CurrentBoard := boardId;
	BoardCreate;
	Board.Tiles[entryX][entryY].Element := E_EMPTY;
	Board.Tiles[entryX][entryY].Color := 0;
	BoardClose;
end;

procedure ReturnToSource;
begin
	BoardChange(0);
	World.Info.BoardTimeSec := 42;
	GamePaused := false;
end;

procedure RunEdge(labelText: string; sourceX, sourceY, dx, dy: integer);
var
	oldX, oldY: integer;
begin
	ReturnToSource;
	oldX := Board.Stats[0].X;
	oldY := Board.Stats[0].Y;
	Board.Tiles[oldX][oldY].Element := E_EMPTY;
	Board.Tiles[oldX][oldY].Color := 0;
	Board.Stats[0].X := sourceX;
	Board.Stats[0].Y := sourceY;
	Board.Tiles[sourceX][sourceY].Element := E_PLAYER;
	Board.Tiles[sourceX][sourceY].Color := ElementDefs[E_PLAYER].Color;
	deltaX := dx;
	deltaY := dy;
	ElementDefs[E_BOARD_EDGE].TouchProc(sourceX + dx, sourceY + dy, 0, deltaX, deltaY);
	PrintState(labelText);
end;

procedure ConfigureTarget(boardId, entryX, entryY: integer; elementAtEntry: byte; playerAtEntry: boolean);
begin
	BoardChange(boardId);
	BoardCreate;
	if playerAtEntry then begin
		Board.Tiles[30][12].Element := E_EMPTY;
		Board.Tiles[30][12].Color := 0;
		Board.Stats[0].X := entryX;
		Board.Stats[0].Y := entryY;
		Board.Tiles[entryX][entryY].Element := E_PLAYER;
		Board.Tiles[entryX][entryY].Color := ElementDefs[E_PLAYER].Color;
	end else begin
		Board.Tiles[entryX][entryY].Element := elementAtEntry;
		Board.Tiles[entryX][entryY].Color := $1F;
	end;
	BoardClose;
	BoardChange(0);
end;

begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	TickTimeDuration := 4;
	New(IoTmpBuf);
	GenerateTransitionTable;
	Assign(output, 'BOARDENT.OUT');
	Rewrite(output);
	WorldCreate;
	World.BoardCount := 4;
	World.BoardData[1] := nil;
	World.BoardData[2] := nil;
	World.BoardData[3] := nil;
	World.BoardData[4] := nil;
	World.BoardLen[1] := 0;
	World.BoardLen[2] := 0;
	World.BoardLen[3] := 0;
	World.BoardLen[4] := 0;
	SetSourceBoard;
	SetDestinationBoard(1, 30, 25);
	SetDestinationBoard(2, 30, 1);
	SetDestinationBoard(3, 60, 12);
	SetDestinationBoard(4, 1, 12);
	BoardChange(0);

	RunEdge('edge-north', 30, 1, 0, -1);
	RunEdge('edge-south', 30, 25, 0, 1);
	RunEdge('edge-west', 1, 12, -1, 0);
	RunEdge('edge-east', 60, 12, 1, 0);

	ConfigureTarget(1, 30, 25, E_SOLID, false);
	RunEdge('edge-blocked', 30, 1, 0, -1);
	ConfigureTarget(1, 30, 25, E_EMPTY, true);
	RunEdge('edge-existing-player', 30, 1, 0, -1);

	Close(output);
	WorldUnload;
	Dispose(IoTmpBuf);
end.
