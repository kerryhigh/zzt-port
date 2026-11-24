program LionProbe;
uses Crt, Video, GameVars, Elements, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
procedure SetupLion(playerX, playerY: byte; seed: longint);
begin
	BoardCreate;
	Board.StatCount := 1;
	Board.Stats[0].X := playerX;
	Board.Stats[0].Y := playerY;
	Board.Tiles[playerX][playerY].Element := E_PLAYER;
	Board.Tiles[playerX][playerY].Color := ElementDefs[E_PLAYER].Color;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].P1 := 0;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := $70;
	Board.Tiles[10][10].Element := E_LION;
	Board.Tiles[10][10].Color := ElementDefs[E_LION].Color;
	World.Info.EnergizerTicks := 0;
	RandSeed := seed;
end;
procedure RunCase(playerX, playerY: byte; seed: longint; randomCount: byte);
var
	i: byte;
begin
	Write(output, 'seed ', seed, ' draws');
	RandSeed := seed;
	for i := 1 to randomCount do begin
		case i of
			1: Write(output, ' ', Random(10));
			2: if playerY = 10 then Write(output, ' ', Random(2)) else Write(output, ' ', Random(3));
			3: Write(output, ' ', Random(2));
		end;
	end;
	Write(output, ' tick');
	SetupLion(playerX, playerY, seed);
	ElementDefs[E_LION].TickProc(1);
	WriteLn(output, ' ', Board.Stats[1].X, ' ', Board.Stats[1].Y,
		' ', Board.Tiles[10][10].Element, ' ', Board.Stats[1].Under.Element);
end;
begin
	New(IoTmpBuf);
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	WorldCreate;
	Assign(output, 'LION.OUT');
	Rewrite(output);
	RunCase(13, 11, 3, 2);
	RunCase(13, 13, 26, 3);
	RunCase(13, 10, 0, 2);
	Close(output);
end.
