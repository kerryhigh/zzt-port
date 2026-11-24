program TransporterTouchProbe;
uses Crt, Video, GameVars, Elements, Sounds, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
	deltaX, deltaY: integer;

procedure SetupTransporters(reverseExit, blockExit: boolean);
begin
	BoardCreate;
	Board.Stats[0].X := 10;
	Board.Stats[0].Y := 10;
	Board.Stats[0].Under.Element := E_EMPTY;
	Board.Stats[0].Under.Color := 0;
	Board.Tiles[10][10].Element := E_PLAYER;
	Board.Tiles[10][10].Color := ElementDefs[E_PLAYER].Color;
	Board.StatCount := 1;
	Board.Stats[1] := StatTemplateDefault;
	Board.Stats[1].X := 11;
	Board.Stats[1].Y := 10;
	Board.Stats[1].StepX := 1;
	Board.Stats[1].StepY := 0;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := 0;
	Board.Tiles[11][10].Element := E_TRANSPORTER;
	Board.Tiles[11][10].Color := $2A;
	if reverseExit then begin
		Board.StatCount := 2;
		Board.Stats[2] := StatTemplateDefault;
		Board.Stats[2].X := 12;
		Board.Stats[2].Y := 10;
		Board.Stats[2].StepX := -1;
		Board.Stats[2].StepY := 0;
		Board.Stats[2].Under.Element := E_EMPTY;
		Board.Stats[2].Under.Color := 0;
		Board.Tiles[12][10].Element := E_TRANSPORTER;
		Board.Tiles[12][10].Color := $2A;
	end else if blockExit then begin
		Board.Tiles[12][10].Element := E_SOLID;
		Board.Tiles[12][10].Color := $1F;
	end;
	Board.Tiles[13][10].Element := E_EMPTY;
	Board.Tiles[13][10].Color := 0;
	SoundCurrentPriority := -1;
	SoundIsPlaying := false;
	SoundBuffer := '';
	deltaX := 1;
	deltaY := 0;
end;

procedure PrintState(labelText: string);
var
	i: integer;
begin
	Write(output, labelText, ' player ', Board.Stats[0].X, ' ', Board.Stats[0].Y,
		' delta ', deltaX, ' ', deltaY, ' sound ', SoundCurrentPriority, ' ', Length(SoundBuffer));
	for i := 1 to Length(SoundBuffer) do
		Write(output, ' ', Ord(SoundBuffer[i]));
	WriteLn(output);
	Flush(output);
end;

begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	TickTimeDuration := 4;
	New(IoTmpBuf);
	Assign(output, 'TRANSPRT.OUT');
	Rewrite(output);
	WorldCreate;
	SoundEnabled := false;
	SoundBlockQueueing := false;

	SetupTransporters(false, false);
	ElementDefs[E_TRANSPORTER].TouchProc(11, 10, 0, deltaX, deltaY);
	PrintState('transport-open');

	SetupTransporters(true, false);
	ElementDefs[E_TRANSPORTER].TouchProc(11, 10, 0, deltaX, deltaY);
	PrintState('transport-reverse-chain');

	SetupTransporters(false, false);
	deltaX := -1;
	ElementDefs[E_TRANSPORTER].TouchProc(11, 10, 0, deltaX, deltaY);
	PrintState('transport-wrong-way');

	SetupTransporters(false, true);
	ElementDefs[E_TRANSPORTER].TouchProc(11, 10, 0, deltaX, deltaY);
	PrintState('transport-blocked');

	Close(output);
	WorldUnload;
	Dispose(IoTmpBuf);
end.
