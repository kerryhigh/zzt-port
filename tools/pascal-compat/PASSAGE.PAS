program PassageTouchProbe;
uses Crt, Video, GameVars, Elements, Sounds, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
	deltaX, deltaY: integer;
	debugOutput: text;

procedure Mark(stage: string);
begin
	Assign(debugOutput, 'PASSAGE.DBG');
	Rewrite(debugOutput);
	WriteLn(debugOutput, stage);
	Close(debugOutput);
end;

procedure PrintState;
var
	i: integer;
begin
	Write(output, 'passage ', World.Info.CurrentBoard, ' player ', Board.Stats[0].X, ' ', Board.Stats[0].Y,
		' start ', Board.Info.StartPlayerX, ' ', Board.Info.StartPlayerY,
		' spawn ', Board.Tiles[30][12].Element, ' ', Board.Tiles[30][12].Color,
		' first ', Board.Tiles[20][15].Element, ' ', Board.Tiles[20][15].Color,
		' last ', Board.Tiles[22][17].Element, ' ', Board.Tiles[22][17].Color,
		' paused ', Ord(GamePaused), ' time ', World.Info.BoardTimeSec,
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
	GenerateTransitionTable;
	Assign(output, 'PASSAGE.OUT');
	Rewrite(output);
	WorldCreate;
	Mark('after-world');

	World.BoardCount := 1;
	World.BoardData[1] := nil;
	World.BoardLen[1] := 0;
	BoardCreate;
	World.Info.CurrentBoard := 1;
	Board.Tiles[20][15].Element := E_PASSAGE;
	Board.Tiles[20][15].Color := $2A;
	Board.Tiles[22][17].Element := E_PASSAGE;
	Board.Tiles[22][17].Color := $2A;
	World.Info.BoardTimeSec := 87;
	BoardClose;
	Mark('after-board1-close');
	BoardChange(0);
	Mark('after-board0-open');

	Board.Tiles[11][10].Element := E_PASSAGE;
	Board.Tiles[11][10].Color := $2A;
	Board.StatCount := 1;
	Board.Stats[1] := StatTemplateDefault;
	Board.Stats[1].X := 11;
	Board.Stats[1].Y := 10;
	Board.Stats[1].P3 := 1;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := 0;

	GameStateElement := E_PLAYER;
	DebugEnabled := false;
	SoundEnabled := false;
	SoundBlockQueueing := false;
	SoundCurrentPriority := -1;
	SoundIsPlaying := false;
	SoundBuffer := '';
	GamePaused := false;
	deltaX := -1;
	deltaY := 1;
	Mark('before-touch');
	ElementDefs[E_PASSAGE].TouchProc(11, 10, 0, deltaX, deltaY);
	Mark('after-touch');
	PrintState;

	Close(output);
	WorldUnload;
	Dispose(IoTmpBuf);
end.
