program ElementDrawProbe;
uses Crt, Video, GameVars, Elements, Game;
var
	output: text;
	phase, direction: integer;
	ch: byte;
	writeCount, lastWriteX, lastWriteY, lastWriteColor, lastWriteChar: integer;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
	writeCount := writeCount + 1;
	lastWriteX := x;
	lastWriteY := y;
	lastWriteColor := color;
	if Length(text) > 0 then
		lastWriteChar := Ord(text[1])
	else
		lastWriteChar := -1;
end;
{$F-}

procedure SetupDraw(element: integer; color: byte; stepX, stepY, cycle: integer);
var
	i: integer;
begin
	BoardCreate;
	for i := 1 to MAX_STAT + 1 do begin
		FillChar(Board.Stats[i], SizeOf(Board.Stats[i]), 0);
		Board.Stats[i].Follower := -1;
		Board.Stats[i].Leader := -1;
	end;
	Board.StatCount := 1;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].StepX := stepX;
	Board.Stats[1].StepY := stepY;
	Board.Stats[1].Cycle := cycle;
	Board.Tiles[10][10].Element := element;
	Board.Tiles[10][10].Color := color;
end;

procedure CaptureStar(labelText: string; startColor: byte);
var
	localPhase: integer;
begin
	SetupDraw(E_STAR, startColor, 0, 0, 1);
	for localPhase := 0 to 3 do begin
		CurrentTick := localPhase;
		ch := 0;
		ElementDefs[E_STAR].DrawProc(10, 10, ch);
		WriteLn(output, 'star-', labelText, ' ', localPhase, ' ', Ord(ch),
			' ', Board.Tiles[10][10].Color);
	end;
end;

procedure CaptureTransporterTick(direction, phase: integer);
var
	stepX, stepY: integer;
begin
	case direction of
		0: begin stepX := 0; stepY := -1; end;
		1: begin stepX := 0; stepY := 1; end;
		2: begin stepX := -1; stepY := 0; end;
	else begin stepX := 1; stepY := 0; end;
	end;
	SetupDraw(E_TRANSPORTER, $0F, stepX, stepY, 2);
	CurrentTick := phase * 2;
	writeCount := 0;
	lastWriteX := -1;
	lastWriteY := -1;
	lastWriteColor := -1;
	lastWriteChar := -1;
	ElementDefs[E_TRANSPORTER].TickProc(1);
	WriteLn(output, 'transporter-tick ', direction, ' ', phase, ' ', writeCount,
		' ', lastWriteX, ' ', lastWriteY, ' ', lastWriteColor, ' ', lastWriteChar);
end;

begin
	New(IoTmpBuf);
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	WorldCreate;
	Assign(output, 'DRAWPRB.OUT');
	Rewrite(output);

	CaptureStar('15', $0F);
	CaptureStar('14', $0E);

	SetupDraw(E_STAR, $FF, 0, 0, 1);
	CurrentTick := 0;
	ch := 0;
	ElementDefs[E_STAR].DrawProc(10, 10, ch);
	WriteLn(output, 'star-255 0 ', Ord(ch), ' ', Board.Tiles[10][10].Color);

	for direction := 0 to 3 do begin
		case direction of
			0: SetupDraw(E_TRANSPORTER, $0F, 0, -1, 2);
			1: SetupDraw(E_TRANSPORTER, $0F, 0, 1, 2);
			2: SetupDraw(E_TRANSPORTER, $0F, -1, 0, 2);
		else SetupDraw(E_TRANSPORTER, $0F, 1, 0, 2);
		end;
		for phase := 0 to 3 do begin
			CurrentTick := phase * 2;
			ch := 0;
			ElementDefs[E_TRANSPORTER].DrawProc(10, 10, ch);
			WriteLn(output, 'transporter ', direction, ' ', phase, ' ', Ord(ch));
		end;
	end;
	for direction := 0 to 3 do
		for phase := 0 to 3 do
			CaptureTransporterTick(direction, phase);

	Close(output);
end.
