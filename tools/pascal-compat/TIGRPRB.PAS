program TigerPrb;
uses Crt, Video, GameVars, Elements, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;

function FindSeed3(value1, value2, value3: integer): longint;
var
	candidate: longint;
	r1, r2, r3: integer;
begin
	FindSeed3 := -1;
	for candidate := 0 to 100000 do begin
		RandSeed := candidate;
		r1 := Random(10);
		if r1 = value1 then begin
			r2 := Random(10);
			if r2 = value2 then begin
				r3 := Random(2);
				if r3 = value3 then begin
					FindSeed3 := candidate;
					exit;
				end;
			end;
		end;
	end;
end;

procedure SetupTiger(playerX, playerY: byte; p2: integer; blockedVertical: boolean);
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
	Board.Stats[1].P2 := p2;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := $70;
	Board.Tiles[10][10].Element := E_TIGER;
	Board.Tiles[10][10].Color := ElementDefs[E_TIGER].Color;
	if blockedVertical then begin
		Board.Tiles[10][11].Element := E_SOLID;
		Board.Tiles[10][11].Color := ElementDefs[E_SOLID].Color;
	end;
	World.Info.EnergizerTicks := 0;
end;

procedure PrintCase(labelText: string; seed: longint);
var
	r1, r2, r3: integer;
begin
	RandSeed := seed;
	r1 := Random(10);
	r2 := Random(10);
	r3 := Random(2);
	RandSeed := seed;
	ElementDefs[E_TIGER].TickProc(1);
	Write(output, labelText, ' seed ', seed, ' draws ', r1, ' ', r2, ' ', r3,
		' count ', Board.StatCount,
		' tiger ', Board.Stats[1].X, ' ', Board.Stats[1].Y,
		' tiles ', Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color,
		' ', Board.Tiles[10][11].Element, ' ', Board.Tiles[10][11].Color,
		' ', Board.Tiles[11][10].Element, ' ', Board.Tiles[11][10].Color);
	if Board.StatCount > 1 then
		Write(output, ' projectile ', Board.Stats[2].X, ' ', Board.Stats[2].Y,
			' ', Board.Stats[2].P1, ' ', Board.Stats[2].StepX, ' ', Board.Stats[2].StepY,
			' ', Board.Stats[2].P2);
	WriteLn(output, ' final-seed ', RandSeed);
end;

var
	blockedSeed, starSeed: longint;
begin
	New(IoTmpBuf);
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	WorldCreate;
	blockedSeed := FindSeed3(0, 0, 0);
	starSeed := FindSeed3(0, 0, 0);
	Assign(output, 'TIGER.OUT');
	Rewrite(output);
	SetupTiger(11, 13, 0, true);
	PrintCase('blocked-vertical', blockedSeed);
	SetupTiger(13, 10, $80, false);
	PrintCase('star-horizontal', starSeed);
	Close(output);
end.
