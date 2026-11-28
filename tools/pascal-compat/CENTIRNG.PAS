program CentipedeRandomCallerProbe;
uses Crt, Video, GameVars, Elements, Game;

{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}

var
	output: text;
	initialSeed: longint;
	previewDraw: integer;
	destinationX, destinationY: integer;

procedure Setup(playerX, playerY: integer);
begin
	BoardCreate;
	Board.StatCount := 1;
	Board.Stats[0].X := playerX;
	Board.Stats[0].Y := playerY;
	Board.Stats[0].Under.Element := E_EMPTY;
	Board.Stats[0].Under.Color := $70;
	Board.Tiles[playerX][playerY].Element := E_PLAYER;
	Board.Tiles[playerX][playerY].Color := ElementDefs[E_PLAYER].Color;

	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].Cycle := 1;
	Board.Stats[1].StepX := 1;
	Board.Stats[1].StepY := 0;
	Board.Stats[1].P1 := 4;
	Board.Stats[1].P2 := 0;
	Board.Stats[1].Leader := -1;
	Board.Stats[1].Follower := -1;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := $70;
	Board.Tiles[10][10].Element := E_CENTIPEDE_HEAD;
	Board.Tiles[10][10].Color := ElementDefs[E_CENTIPEDE_HEAD].Color;
	World.Info.EnergizerTicks := 0;

	if playerX = 10 then begin
		destinationX := 10;
		destinationY := 9;
	end else begin
		destinationX := 9;
		destinationY := 10;
	end;
end;

procedure RunCase(labelText: string; playerX, playerY: integer);
begin
	Setup(playerX, playerY);
	initialSeed := 0;
	RandSeed := initialSeed;
	previewDraw := Random(10);
	RandSeed := initialSeed;
	ElementDefs[E_CENTIPEDE_HEAD].TickProc(1);
	WriteLn(output, labelText,
		' ', (initialSeed and $FFFF), ' ', ((initialSeed shr 16) and $FFFF),
		' 1 10 ', previewDraw,
		' ', (RandSeed and $FFFF), ' ', ((RandSeed shr 16) and $FFFF),
		' ', Board.Stats[1].X, ' ', Board.Stats[1].Y,
		' ', Board.Stats[1].StepX, ' ', Board.Stats[1].StepY,
		' ', Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color,
		' ', destinationX, ' ', destinationY,
		' ', Board.Tiles[destinationX][destinationY].Element,
		' ', Board.Tiles[destinationX][destinationY].Color);
end;

begin
	New(IoTmpBuf);
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	WorldCreate;
	Assign(output, 'CENTIRNG.OUT');
	Rewrite(output);
	{ The X-aligned case reaches ELEMENTS.PAS:205 and consumes one Random(10). }
	RunCase('x-aligned', 10, 6);
	{ The first condition is short-circuited; the Y-aligned case reaches line 208. }
	RunCase('y-aligned', 2, 10);
	Close(output);
	WorldUnload;
	Dispose(IoTmpBuf);
end.
