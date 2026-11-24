program SlimeProbe;
uses Crt, Video, GameVars, Elements, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;

procedure SetupSlime(p1, p2: integer; openMask: byte);
var
	dir, x, y, statId: integer;
begin
	BoardCreate;
	for statId := 1 to MAX_STAT + 1 do begin
		FillChar(Board.Stats[statId], SizeOf(Board.Stats[statId]), 0);
		Board.Stats[statId].Follower := -1;
		Board.Stats[statId].Leader := -1;
	end;
	Board.StatCount := 1;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].Cycle := ElementDefs[E_SLIME].Cycle;
	Board.Stats[1].P1 := p1;
	Board.Stats[1].P2 := p2;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := $70;
	Board.Tiles[10][10].Element := E_SLIME;
	Board.Tiles[10][10].Color := $2A;
	for dir := 0 to 3 do begin
		case dir of
			0: begin x := 10; y := 9; end;
			1: begin x := 10; y := 11; end;
			2: begin x := 9; y := 10; end;
		else begin x := 11; y := 10; end;
		end;
		if (openMask and (1 shl dir)) <> 0 then begin
			Board.Tiles[x][y].Element := E_EMPTY;
			Board.Tiles[x][y].Color := $30 + dir;
		end else begin
			Board.Tiles[x][y].Element := E_SOLID;
			Board.Tiles[x][y].Color := $40 + dir;
		end;
	end;
end;

procedure WriteState(caseId: integer);
var
	i, x, y, dir: integer;
begin
	Write(output, caseId, ' ', Board.StatCount);
	for i := 1 to 4 do begin
		if i <= Board.StatCount then begin
			with Board.Stats[i] do
				Write(output, ' ', X, ' ', Y, ' ', Cycle, ' ', P1, ' ', P2,
					' ', Under.Element, ' ', Under.Color);
		end else
			Write(output, ' -1 -1 -1 -1 -1 -1 -1');
	end;
	for dir := -1 to 3 do begin
		case dir of
			-1: begin x := 10; y := 10; end;
			0: begin x := 10; y := 9; end;
			1: begin x := 10; y := 11; end;
			2: begin x := 9; y := 10; end;
		else begin x := 11; y := 10; end;
		end;
		Write(output, ' ', Board.Tiles[x][y].Element, ' ', Board.Tiles[x][y].Color);
	end;
	WriteLn(output);
	Flush(output);
end;

begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	New(IoTmpBuf);
	WorldCreate;
	Assign(output, 'SLIMEPRB.OUT');
	Rewrite(output);

	SetupSlime(1, 2, 15);
	ElementDefs[E_SLIME].TickProc(1);
	WriteState(1);

	SetupSlime(2, 2, 15);
	ElementDefs[E_SLIME].TickProc(1);
	WriteState(2);

	SetupSlime(3, 2, 8);
	ElementDefs[E_SLIME].TickProc(1);
	WriteState(3);

	SetupSlime(2, 2, 0);
	ElementDefs[E_SLIME].TickProc(1);
	WriteState(4);

	SetupSlime(2, 2, 3);
	ElementDefs[E_SLIME].TickProc(1);
	ElementDefs[E_SLIME].TickProc(2);
	WriteState(5);

	Close(output);
	Dispose(IoTmpBuf);
end.
