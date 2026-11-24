program SpinningGunProbe;
uses Crt, Video, GameVars, Elements, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
procedure RunCase(labelText: string; playerX, playerY, gunX, gunY, p1, p2, seed: integer);
var
	projectileId: integer;
begin
	BoardCreate;
	Board.StatCount := 1;
	Board.Stats[0].X := playerX;
	Board.Stats[0].Y := playerY;
	Board.Tiles[playerX][playerY].Element := E_PLAYER;
	Board.Tiles[playerX][playerY].Color := ElementDefs[E_PLAYER].Color;
	Board.Stats[1].X := gunX;
	Board.Stats[1].Y := gunY;
	Board.Stats[1].P1 := p1;
	Board.Stats[1].P2 := p2;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := $70;
	Board.Tiles[gunX][gunY].Element := E_SPINNING_GUN;
	Board.Tiles[gunX][gunY].Color := ElementDefs[E_SPINNING_GUN].Color;
	RandSeed := seed;
	ElementDefs[E_SPINNING_GUN].TickProc(1);
	Write(output, labelText, ' stats ', Board.StatCount,
		' player ', Board.Stats[0].X, ' ', Board.Stats[0].Y,
		' gun ', Board.Stats[1].X, ' ', Board.Stats[1].Y,
		' rng ', RandSeed);
	if Board.StatCount > 1 then begin
		projectileId := Board.StatCount;
		with Board.Stats[projectileId] do
			Write(output, ' projectile ', Board.Tiles[X][Y].Element, ' ', X, ' ', Y,
				' step ', StepX, ' ', StepY, ' p1 ', P1, ' p2 ', P2,
				' under ', Under.Element, ' ', Under.Color);
	end else
		Write(output, ' no-projectile');
	WriteLn(output);
end;
begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	New(IoTmpBuf);
	WorldCreate;
	Assign(output, 'SPINGUN.OUT');
	Rewrite(output);
	RunCase('aimed-vertical-bullet', 10, 20, 12, 10, 8, 1, 0);
	RunCase('aimed-same-row-bullet', 20, 10, 10, 12, 8, 1, 0);
	RunCase('vertical-bullet', 10, 20, 12, 10, 0, 1, 0);
	RunCase('same-row-bullet', 20, 10, 10, 12, 0, 1, 0);
	RunCase('vertical-star', 10, 20, 12, 10, 0, 129, 0);
	RunCase('fire-rate-zero', 10, 20, 12, 10, 0, 0, 0);
	Close(output);
end.
