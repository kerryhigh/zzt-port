program BombTouchProbe;
uses Crt, Video, GameVars, Elements, Sounds, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
	debugOutput: text;
	deltaX, deltaY: integer;
    bombSeed: longint;

procedure Mark(stage: string);
begin
	Assign(debugOutput, 'BOMBDBG.OUT');
	Rewrite(debugOutput);
	WriteLn(debugOutput, stage);
	Close(debugOutput);
end;

function FindRandom7ZeroSeed: longint;
var
	candidate: longint;
begin
	FindRandom7ZeroSeed := -1;
	for candidate := 0 to 100000 do begin
		RandSeed := candidate;
		if Random(7) = 0 then begin
			FindRandom7ZeroSeed := candidate;
			exit;
		end;
	end;
end;

procedure SetupBombTick(fuse: integer);
var
	x, y: integer;
begin
	BoardCreate;
	for x := 5 to 15 do
		for y := 5 to 15 do begin
			Board.Tiles[x][y].Element := E_SOLID;
			Board.Tiles[x][y].Color := $46;
		end;
	Board.StatCount := 1;
	Board.Stats[0].X := 30;
	Board.Stats[0].Y := 12;
	Board.Tiles[30][12].Element := E_PLAYER;
	Board.Tiles[30][12].Color := ElementDefs[E_PLAYER].Color;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].Cycle := 6;
	Board.Stats[1].P1 := fuse;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := $70;
	Board.Tiles[10][10].Element := E_BOMB;
	Board.Tiles[10][10].Color := $28;
	Board.Tiles[11][10].Element := E_EMPTY;
	Board.Tiles[11][10].Color := $50;
	SoundCurrentPriority := -1;
	SoundIsPlaying := false;
	SoundBlockQueueing := false;
	SoundBuffer := '';
end;

procedure WriteBombTick(caseId: integer);
var
	i: integer;
begin
	Write(output, caseId, ' ', Board.StatCount);
	if Board.StatCount > 0 then
		Write(output, ' ', Board.Stats[1].P1, ' ', Board.Stats[1].Under.Element,
			' ', Board.Stats[1].Under.Color)
	else
		Write(output, ' -1 -1 -1');
	Write(output, ' ', Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color,
		' ', Board.Tiles[11][10].Element, ' ', Board.Tiles[11][10].Color,
		' ', SoundCurrentPriority, ' ', Length(SoundBuffer));
	for i := 1 to 12 do begin
		if i <= Length(SoundBuffer) then
			Write(output, ' ', Ord(SoundBuffer[i]))
		else
			Write(output, ' -1');
	end;
	WriteLn(output);
end;

begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	TickTimeDuration := 4;
	New(IoTmpBuf);
	Assign(output, 'BOMB.OUT');
	Rewrite(output);
	Mark('before-world');
	WorldCreate;
	Mark('after-world');
	GameStateElement := E_PLAYER;
	DebugEnabled := false;
	SoundEnabled := false;
	SoundBlockQueueing := false;
	SoundCurrentPriority := -1;
	SoundIsPlaying := false;
	SoundBuffer := '';
	Mark('after-init');
	WriteLn(output, 'bomb-state');
	Flush(output);

	Board.StatCount := 1;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].Cycle := 6;
	Board.Stats[1].P1 := 0;
	Board.Stats[1].P2 := 0;
	Board.Stats[1].P3 := 0;
	Board.Stats[1].Follower := -1;
	Board.Stats[1].Leader := -1;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := $70;
	Board.Stats[1].Data := nil;
	Board.Stats[1].DataPos := 0;
	Board.Stats[1].DataLen := 0;
	Board.Tiles[10][10].Element := E_BOMB;
	Board.Tiles[10][10].Color := $28;
	deltaX := 1;
	deltaY := 0;
	Mark('before-touch');
	ElementDefs[E_BOMB].TouchProc(10, 10, 0, deltaX, deltaY);
	Mark('after-touch');
	if Board.Stats[1].P1 <> 9 then Halt(1);
	Mark('p1-ok');
	if Board.Tiles[10][10].Element <> E_BOMB then Halt(2);
	Mark('tile-ok');
	if Board.Tiles[10][10].Color <> $28 then Halt(3);
	Mark('color-ok');
	if Board.Info.Message <> 'Bomb activated!' then Halt(4);
	Mark('message-ok');
	if Board.StatCount <> 2 then Halt(5);
	Mark('count-ok');
	if (Board.Stats[2].X <> 0) or (Board.Stats[2].Y <> 0)
		or (Board.Stats[2].P2 <> 40) then Halt(6);
	Mark('timer-ok');
	if (SoundCurrentPriority <> 4) or (Length(SoundBuffer) <> 10) then Halt(7);
	Mark('sound-size-ok');
	if (Ord(SoundBuffer[1]) <> 48) or (Ord(SoundBuffer[2]) <> 1)
		or (Ord(SoundBuffer[3]) <> 53) or (Ord(SoundBuffer[4]) <> 1)
		or (Ord(SoundBuffer[5]) <> 64) or (Ord(SoundBuffer[6]) <> 1)
		or (Ord(SoundBuffer[7]) <> 69) or (Ord(SoundBuffer[8]) <> 1)
		or (Ord(SoundBuffer[9]) <> 80) or (Ord(SoundBuffer[10]) <> 1) then Halt(8);
	Mark('sound-data-ok');
	WriteLn(output, 'bomb-start ', Board.Stats[1].P1, ' ',
		Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color, ' ',
		Length(Board.Info.Message), ' ', Board.StatCount, ' ',
		Board.Stats[2].X, ' ', Board.Stats[2].Y, ' ', Board.Stats[2].P2, ' ',
		SoundCurrentPriority, ' ', Length(SoundBuffer), ' ',
		Ord(SoundBuffer[1]), ' ', Ord(SoundBuffer[2]), ' ',
		Ord(SoundBuffer[3]), ' ', Ord(SoundBuffer[4]), ' ',
		Ord(SoundBuffer[5]), ' ', Ord(SoundBuffer[6]), ' ',
		Ord(SoundBuffer[7]), ' ', Ord(SoundBuffer[8]), ' ',
		Ord(SoundBuffer[9]), ' ', Ord(SoundBuffer[10]), ' ', deltaX, ' ', deltaY);
	Flush(output);

	BoardCreate;
	Board.StatCount := 1;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].P1 := 9;
	Board.Stats[1].Follower := -1;
	Board.Stats[1].Leader := -1;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := $70;
	Board.Stats[1].Data := nil;
	Board.Stats[1].DataLen := 0;
	Board.Tiles[10][10].Element := E_BOMB;
	Board.Tiles[10][10].Color := $28;
	Board.Tiles[11][10].Element := E_EMPTY;
	Board.Tiles[11][10].Color := $31;
	SoundCurrentPriority := -1;
	SoundIsPlaying := false;
	SoundBuffer := '';
	deltaX := 1;
	deltaY := 0;
	Mark('before-push');
	ElementDefs[E_BOMB].TouchProc(10, 10, 0, deltaX, deltaY);
	Mark('after-push');
	if Board.Stats[1].P1 <> 9 then Halt(9);
	if (Board.Stats[1].X <> 11) or (Board.Stats[1].Y <> 10) then Halt(10);
	if Board.Tiles[10][10].Element <> E_EMPTY then Halt(11);
	if Board.Tiles[10][10].Color <> $70 then Halt(12);
	if Board.Tiles[11][10].Element <> E_BOMB then Halt(13);
	if Board.Tiles[11][10].Color <> $08 then Halt(14);
	if (Board.Stats[1].Under.Element <> E_EMPTY) or (Board.Stats[1].Under.Color <> $31) then Halt(15);
	if (SoundCurrentPriority <> -1) or (Length(SoundBuffer) <> 0) then Halt(16);
	WriteLn(output, 'bomb-push ', Board.Stats[1].P1, ' ', Board.Stats[1].X, ' ', Board.Stats[1].Y, ' ',
		Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color, ' ',
		Board.Tiles[11][10].Element, ' ', Board.Tiles[11][10].Color, ' ',
		Board.Stats[1].Under.Element, ' ', Board.Stats[1].Under.Color, ' ',
		SoundCurrentPriority, ' ', Length(SoundBuffer), ' ', deltaX, ' ', deltaY);
	Flush(output);

	BoardCreate;
	Board.StatCount := 1;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].P1 := 9;
	Board.Stats[1].Follower := -1;
	Board.Stats[1].Leader := -1;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := $70;
	Board.Stats[1].Data := nil;
	Board.Stats[1].DataLen := 0;
	Board.Tiles[10][10].Element := E_BOMB;
	Board.Tiles[10][10].Color := $28;
	Board.Tiles[11][10].Element := E_NORMAL;
	Board.Tiles[11][10].Color := $0E;
	SoundCurrentPriority := -1;
	SoundIsPlaying := false;
	SoundBuffer := '';
	deltaX := 1;
	deltaY := 0;
	ElementDefs[E_BOMB].TouchProc(10, 10, 0, deltaX, deltaY);
	if Board.Stats[1].P1 <> 9 then Halt(17);
	if (Board.Stats[1].X <> 10) or (Board.Stats[1].Y <> 10) then Halt(18);
	if (Board.Tiles[10][10].Element <> E_BOMB) or (Board.Tiles[10][10].Color <> $28) then Halt(19);
	if (Board.Tiles[11][10].Element <> E_NORMAL) or (Board.Tiles[11][10].Color <> $0E) then Halt(20);
	if (SoundCurrentPriority <> -1) or (Length(SoundBuffer) <> 0) then Halt(21);
	WriteLn(output, 'bomb-blocked ', Board.Stats[1].P1, ' ', Board.Stats[1].X, ' ', Board.Stats[1].Y, ' ',
		Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color, ' ',
		Board.Tiles[11][10].Element, ' ', Board.Tiles[11][10].Color, ' ',
		SoundCurrentPriority, ' ', Length(SoundBuffer), ' ', deltaX, ' ', deltaY);

	bombSeed := FindRandom7ZeroSeed;
	if bombSeed < 0 then Halt(22);
	SetupBombTick(0);
	ElementDefs[E_BOMB].TickProc(1);
	Write(output, 'bomb-tick ');
	WriteBombTick(1);

	SetupBombTick(3);
	ElementDefs[E_BOMB].TickProc(1);
	Write(output, 'bomb-tick ');
	WriteBombTick(2);
	SoundCurrentPriority := -1;
	SoundBuffer := '';
	RandSeed := bombSeed;
	ElementDefs[E_BOMB].TickProc(1);
	Write(output, 'bomb-tick ');
	WriteBombTick(3);
	SoundCurrentPriority := -1;
	SoundBuffer := '';
	ElementDefs[E_BOMB].TickProc(1);
	Write(output, 'bomb-tick ');
	WriteBombTick(4);

	SetupBombTick(-1);
	ElementDefs[E_BOMB].TickProc(1);
	Write(output, 'bomb-tick ');
	WriteBombTick(5);

	Close(output);
	Dispose(IoTmpBuf);
end.
