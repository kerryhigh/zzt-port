program PusherAndDuplicatorTickProbe;
uses Crt, Video, GameVars, Elements, Sounds, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
	i: integer;

procedure ResetBoard;
begin
	BoardCreate;
	FillChar(Board.Stats, SizeOf(Board.Stats), 0);
	for i := 0 to MAX_STAT + 1 do begin
		Board.Stats[i].Follower := -1;
		Board.Stats[i].Leader := -1;
	end;
	SoundCurrentPriority := -1;
	SoundIsPlaying := false;
	SoundBlockQueueing := false;
	SoundBuffer := '';
end;

procedure SetStat(statId, x, y: integer; element: byte; color: byte);
begin
	Board.Stats[statId].X := x;
	Board.Stats[statId].Y := y;
	Board.Stats[statId].Under.Element := E_EMPTY;
	Board.Stats[statId].Under.Color := $70;
	Board.Tiles[x][y].Element := element;
	Board.Tiles[x][y].Color := color;
end;

procedure PrintStat(statId: integer);
begin
	if statId <= Board.StatCount then
		Write(output, ' ', Board.Stats[statId].X, ' ', Board.Stats[statId].Y,
			' ', Board.Stats[statId].StepX, ' ', Board.Stats[statId].StepY,
			' ', Board.Stats[statId].Cycle, ' ', Board.Stats[statId].P1,
			' ', Board.Stats[statId].P2, ' ', Board.Stats[statId].P3,
			' ', Board.Stats[statId].Follower, ' ', Board.Stats[statId].Leader,
			' ', Board.Stats[statId].Under.Element, ' ', Board.Stats[statId].Under.Color)
	else
		Write(output, ' -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1');
end;

function SoundByte(index: integer): integer;
begin
	if index <= Length(SoundBuffer) then
		SoundByte := Ord(SoundBuffer[index])
	else
		SoundByte := -1;
end;

procedure PrintSnapshot(kind: string; caseId: integer; extraStatId: integer);
var
	x: integer;
	statCount: integer;
begin
	statCount := Board.StatCount;
	Write(output, kind, ' ', caseId, ' ', statCount);
	PrintStat(1);
	PrintStat(2);
	if extraStatId > 0 then begin
		Write(output, ' ', Board.Stats[extraStatId].X, ' ', Board.Stats[extraStatId].Y,
			' ', Board.Stats[extraStatId].StepX, ' ', Board.Stats[extraStatId].StepY,
			' ', Board.Stats[extraStatId].Cycle, ' ', Board.Stats[extraStatId].P1,
			' ', Board.Stats[extraStatId].P2, ' ', Board.Stats[extraStatId].P3,
			' ', Board.Stats[extraStatId].Follower, ' ', Board.Stats[extraStatId].Leader,
			' ', Board.Stats[extraStatId].Under.Element, ' ', Board.Stats[extraStatId].Under.Color);
	end else begin
		Write(output, ' -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1');
	end;
	for x := 9 to 14 do
		Write(output, ' ', Board.Tiles[x][10].Element, ' ', Board.Tiles[x][10].Color);
	Write(output, ' ', SoundCurrentPriority, ' ', Ord(SoundIsPlaying), ' ', Length(SoundBuffer));
	for x := 1 to 8 do
		Write(output, ' ', SoundByte(x));
	WriteLn(output);
	Flush(output);
end;

procedure SetupDuplicator(statCount, sourceElement: integer);
begin
	ResetBoard;
	Board.StatCount := statCount;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].StepX := 1;
	Board.Stats[1].P1 := 5;
	Board.Stats[1].P2 := 2;
	Board.Stats[1].Cycle := 9;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := $70;
	Board.Tiles[10][10].Element := E_DUPLICATOR;
	Board.Tiles[10][10].Color := $0E;
	if sourceElement = E_OBJECT then begin
		Board.Stats[2].X := 11;
		Board.Stats[2].Y := 10;
		Board.Stats[2].Cycle := 4;
		Board.Stats[2].P1 := 6;
		Board.Stats[2].P2 := 7;
		Board.Stats[2].P3 := 8;
		Board.Stats[2].StepX := -1;
		Board.Stats[2].StepY := 1;
		Board.Stats[2].Under.Element := E_EMPTY;
		Board.Stats[2].Under.Color := $70;
	end;
	Board.Tiles[11][10].Element := sourceElement;
	Board.Tiles[11][10].Color := $2A;
	Board.Tiles[9][10].Element := E_EMPTY;
	Board.Tiles[9][10].Color := $70;
end;

procedure SetupPusher(blocked: boolean);
begin
	ResetBoard;
	Board.StatCount := 2;
	SetStat(1, 10, 10, E_PUSHER, $0F);
	Board.Stats[1].StepX := 1;
	SetStat(2, 11, 10, E_PUSHER, $0F);
	Board.Stats[2].StepX := 1;
	Board.Tiles[12][10].Element := E_BOULDER;
	Board.Tiles[12][10].Color := $0E;
	Board.Tiles[13][10].Element := E_EMPTY;
	Board.Tiles[13][10].Color := $70;
	Board.Tiles[14][10].Element := E_EMPTY;
	Board.Tiles[14][10].Color := $70;
	if blocked then begin
		Board.Tiles[13][10].Element := E_SOLID;
		Board.Tiles[13][10].Color := $0F;
	end;
end;

begin
	VideoWriteText := QuietWrite;
	New(IoTmpBuf);
	WorldCreate;
	Assign(output, 'PUSHDUP.OUT');
	Rewrite(output);

	SetupDuplicator(2, E_OBJECT);
	Board.Stats[1].P1 := 4;
	ElementDefs[E_DUPLICATOR].TickProc(1);
	PrintSnapshot('dup-tick', 1, 0);

	SetupDuplicator(2, E_OBJECT);
	ElementDefs[E_DUPLICATOR].TickProc(1);
	PrintSnapshot('dup-tick', 2, Board.StatCount);

	SetupDuplicator(1, E_BOULDER);
	ElementDefs[E_DUPLICATOR].TickProc(1);
	PrintSnapshot('dup-tick', 3, 0);

	SetupDuplicator(MAX_STAT - 1, E_OBJECT);
	ElementDefs[E_DUPLICATOR].TickProc(1);
	PrintSnapshot('dup-tick', 4, Board.StatCount);

	SetupDuplicator(MAX_STAT, E_OBJECT);
	ElementDefs[E_DUPLICATOR].TickProc(1);
	PrintSnapshot('dup-tick', 5, 0);

	SetupPusher(false);
	ElementDefs[E_PUSHER].TickProc(2);
	PrintSnapshot('pusher-tick', 1, 0);

	SetupPusher(true);
	ElementDefs[E_PUSHER].TickProc(2);
	PrintSnapshot('pusher-tick', 2, 0);

	Close(output);
	WorldUnload;
	Dispose(IoTmpBuf);
end.
