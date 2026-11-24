program CentipPr;
uses Crt, Video, GameVars, Elements, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;

function FindSeed3(max1, value1, max2, value2, max3, value3: integer): longint;
var
	candidate: longint;
	r1, r2, r3: integer;
begin
	FindSeed3 := -1;
	for candidate := 0 to 100000 do begin
		RandSeed := candidate;
		r1 := Random(max1);
		if r1 = value1 then begin
			r2 := Random(max2);
			if r2 = value2 then begin
				r3 := Random(max3);
				if r3 = value3 then begin
					FindSeed3 := candidate;
					exit;
				end;
			end;
		end;
	end;
end;

procedure SetupCentipede(chainLength: byte; blocked: boolean);
var
	i: byte;
	x: byte;
begin
	BoardCreate;
	Board.StatCount := chainLength;
	Board.Stats[0].X := 2;
	Board.Stats[0].Y := 2;
	Board.Tiles[2][2].Element := E_PLAYER;
	Board.Tiles[2][2].Color := ElementDefs[E_PLAYER].Color;
	for i := 1 to chainLength do begin
		x := 11 - i;
		Board.Stats[i].X := x;
		Board.Stats[i].Y := 10;
		Board.Stats[i].Cycle := 1;
		Board.Stats[i].StepX := 1;
		Board.Stats[i].StepY := 0;
		Board.Stats[i].P1 := 4;
		Board.Stats[i].P2 := 0;
		Board.Stats[i].Leader := i - 1;
		Board.Stats[i].Follower := i + 1;
		Board.Stats[i].Under.Element := E_EMPTY;
		Board.Stats[i].Under.Color := $70;
		if i = 1 then begin
			Board.Stats[i].Leader := -1;
			Board.Tiles[x][10].Element := E_CENTIPEDE_HEAD;
		end else begin
			Board.Tiles[x][10].Element := E_CENTIPEDE_SEGMENT;
		end;
		Board.Tiles[x][10].Color := ElementDefs[Board.Tiles[x][10].Element].Color;
	end;
	Board.Stats[chainLength].Follower := -1;
	if blocked then begin
		Board.Tiles[11][10].Element := E_SOLID;
		Board.Tiles[10][9].Element := E_SOLID;
		Board.Tiles[10][11].Element := E_SOLID;
	end else begin
		Board.Tiles[11][10].Element := E_EMPTY;
		Board.Tiles[11][10].Color := $70;
	end;
	World.Info.EnergizerTicks := 0;
end;

procedure PrintChain(labelText: string; seed: longint; draws: string);
begin
	Write(output, labelText, ' seed ', seed, ' draws ', draws,
		' count ', Board.StatCount);
	if Board.StatCount >= 1 then
		Write(output, ' s1 ', Board.Stats[1].X, ' ', Board.Stats[1].Y,
			' ', Board.Stats[1].StepX, ' ', Board.Stats[1].StepY,
			' ', Board.Stats[1].Leader, ' ', Board.Stats[1].Follower,
			' ', Board.Stats[1].P1, ' ', Board.Stats[1].P2);
	if Board.StatCount >= 2 then
		Write(output, ' s2 ', Board.Stats[2].X, ' ', Board.Stats[2].Y,
			' ', Board.Stats[2].StepX, ' ', Board.Stats[2].StepY,
			' ', Board.Stats[2].Leader, ' ', Board.Stats[2].Follower,
			' ', Board.Stats[2].P1, ' ', Board.Stats[2].P2);
	if Board.StatCount >= 3 then
		Write(output, ' s3 ', Board.Stats[3].X, ' ', Board.Stats[3].Y,
			' ', Board.Stats[3].StepX, ' ', Board.Stats[3].StepY,
			' ', Board.Stats[3].Leader, ' ', Board.Stats[3].Follower,
			' ', Board.Stats[3].P1, ' ', Board.Stats[3].P2);
	WriteLn(output, ' row ', Board.Tiles[8][10].Element, ' ', Board.Tiles[8][10].Color,
		' ', Board.Tiles[9][10].Element, ' ', Board.Tiles[9][10].Color,
		' ', Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color,
		' ', Board.Tiles[11][10].Element, ' ', Board.Tiles[11][10].Color,
		' seed-after ', RandSeed);
end;

var
	chainSeed, reverseSeed: longint;
begin
	New(IoTmpBuf);
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	WorldCreate;
	chainSeed := 0;
	reverseSeed := FindSeed3(10, 0, 2, 0, 2, 0);
	Assign(output, 'CENTIP.OUT');
	Rewrite(output);
	SetupCentipede(2, false);
	RandSeed := chainSeed;
	ElementDefs[E_CENTIPEDE_HEAD].TickProc(1);
	PrintChain('follow', chainSeed, '0');
	SetupCentipede(3, true);
	RandSeed := reverseSeed;
	ElementDefs[E_CENTIPEDE_HEAD].TickProc(1);
	PrintChain('reverse', reverseSeed, '0 0 0');
	SetupCentipede(1, false);
	RandSeed := 0;
	Board.Stats[1].Leader := -1;
	Board.Stats[1].Follower := -1;
	Board.Tiles[10][10].Element := E_CENTIPEDE_SEGMENT;
	Board.Tiles[10][10].Color := ElementDefs[E_CENTIPEDE_SEGMENT].Color;
	ElementDefs[E_CENTIPEDE_SEGMENT].TickProc(1);
	PrintChain('segment-pending', RandSeed, '');
	ElementDefs[E_CENTIPEDE_SEGMENT].TickProc(1);
	PrintChain('segment-head', RandSeed, '');
	Close(output);
end.
