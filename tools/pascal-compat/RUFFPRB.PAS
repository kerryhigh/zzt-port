program RuffPrb;
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

function FindSeed2(max1, value1, max2, value2: integer): longint;
var
	candidate: longint;
	r1, r2: integer;
begin
	FindSeed2 := -1;
	for candidate := 0 to 100000 do begin
		RandSeed := candidate;
		r1 := Random(max1);
		if r1 = value1 then begin
			r2 := Random(max2);
			if r2 = value2 then begin
				FindSeed2 := candidate;
				exit;
			end;
		end;
	end;
end;

procedure SetupRuffian;
begin
	BoardCreate;
	Board.StatCount := 1;
	Board.Stats[0].X := 20;
	Board.Stats[0].Y := 10;
	Board.Tiles[20][10].Element := E_PLAYER;
	Board.Tiles[20][10].Color := ElementDefs[E_PLAYER].Color;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].P1 := 8;
	Board.Stats[1].P2 := 8;
	Board.Stats[1].StepX := 0;
	Board.Stats[1].StepY := 0;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := $70;
	Board.Tiles[10][10].Element := E_RUFFIAN;
	Board.Tiles[10][10].Color := ElementDefs[E_RUFFIAN].Color;
	World.Info.EnergizerTicks := 0;
end;

procedure PrintRuffian(labelText: string; draws: string);
begin
	WriteLn(output, labelText, ' draws ', draws,
		' player ', Board.Stats[0].X, ' ', Board.Stats[0].Y,
		' ruffian ', Board.Stats[1].X, ' ', Board.Stats[1].Y,
		' step ', Board.Stats[1].StepX, ' ', Board.Stats[1].StepY,
		' under ', Board.Stats[1].Under.Element, ' ', Board.Stats[1].Under.Color,
		' tiles ', Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color,
		' ', Board.Tiles[11][10].Element, ' ', Board.Tiles[11][10].Color,
		' ', Board.Tiles[12][10].Element, ' ', Board.Tiles[12][10].Color);
end;

var
	wakeSeed, moveSeed, stopSeed: longint;
begin
	New(IoTmpBuf);
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	WorldCreate;
	wakeSeed := FindSeed3(17, 16, 9, 8, 2, 0);
	moveSeed := FindSeed3(9, 0, 2, 0, 17, 0);
	stopSeed := FindSeed2(9, 8, 17, 16);
	Assign(output, 'RUFFIAN.OUT');
	Rewrite(output);
	WriteLn(output, 'seeds ', wakeSeed, ' ', moveSeed, ' ', stopSeed);
	SetupRuffian;
	RandSeed := wakeSeed;
	ElementDefs[E_RUFFIAN].TickProc(1);
	PrintRuffian('wake', '16 8 0');
	RandSeed := moveSeed;
	ElementDefs[E_RUFFIAN].TickProc(1);
	PrintRuffian('move-retain', '0 0 0');
	Board.Stats[1].P1 := 0;
	RandSeed := stopSeed;
	ElementDefs[E_RUFFIAN].TickProc(1);
	PrintRuffian('move-stop', '8 16');
	Close(output);
end.
