program TerrainTouchProbe;
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

procedure Mark(stage: string);
begin
	Assign(debugOutput, 'TERRAIN.DBG');
	Rewrite(debugOutput);
	WriteLn(debugOutput, stage);
	Close(debugOutput);
end;

procedure ResetScene;
begin
	BoardCreate;
	SoundCurrentPriority := -1;
	SoundIsPlaying := false;
	SoundBuffer := '';
	deltaX := 2;
	deltaY := -1;
end;

begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	TickTimeDuration := 4;
	New(IoTmpBuf);
	Assign(output, 'TERRAIN.OUT');
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
	WriteLn(output, 'terrain-state');
	Flush(output);

	ResetScene;
	Board.Tiles[10][10].Element := E_INVISIBLE;
	Board.Tiles[10][10].Color := $2A;
	ElementDefs[E_INVISIBLE].TouchProc(10, 10, 0, deltaX, deltaY);
	WriteLn(output, 'invisible ', Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color, ' ',
		Length(Board.Info.Message), ' ', Board.StatCount, ' ', Board.Stats[1].X, ' ', Board.Stats[1].Y, ' ',
		Board.Stats[1].P2, ' ', SoundCurrentPriority, ' ', Length(SoundBuffer), ' ',
		Ord(SoundBuffer[1]), ' ', Ord(SoundBuffer[2]), ' ', Ord(SoundBuffer[3]), ' ', Ord(SoundBuffer[4]), ' ',
	deltaX, ' ', deltaY);
	Flush(output);

	ResetScene;
	MessageForestNotShown := true;
	Board.Tiles[10][10].Element := E_FOREST;
	Board.Tiles[10][10].Color := $2B;
	ElementDefs[E_FOREST].TouchProc(10, 10, 0, deltaX, deltaY);
	WriteLn(output, 'forest-first ', Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color, ' ',
		Length(Board.Info.Message), ' ', Ord(MessageForestNotShown), ' ', Board.StatCount, ' ',
		Board.Stats[1].P2, ' ', SoundCurrentPriority, ' ', Length(SoundBuffer), ' ',
		Ord(SoundBuffer[1]), ' ', Ord(SoundBuffer[2]), ' ', deltaX, ' ', deltaY);
	Flush(output);

	ResetScene;
	MessageForestNotShown := false;
	Board.Tiles[10][10].Element := E_FOREST;
	Board.Tiles[10][10].Color := $2B;
	ElementDefs[E_FOREST].TouchProc(10, 10, 0, deltaX, deltaY);
	WriteLn(output, 'forest-repeat ', Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color, ' ',
		Length(Board.Info.Message), ' ', Ord(MessageForestNotShown), ' ', Board.StatCount, ' ',
		SoundCurrentPriority, ' ', Length(SoundBuffer), ' ', Ord(SoundBuffer[1]), ' ', Ord(SoundBuffer[2]), ' ',
		deltaX, ' ', deltaY);
	Flush(output);

	ResetScene;
	MessageFakeNotShown := true;
	Board.Tiles[10][10].Element := E_FAKE;
	Board.Tiles[10][10].Color := $2C;
	ElementDefs[E_FAKE].TouchProc(10, 10, 0, deltaX, deltaY);
	WriteLn(output, 'fake-first ', Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color, ' ',
		Length(Board.Info.Message), ' ', Ord(MessageFakeNotShown), ' ', Board.StatCount, ' ',
		Board.Stats[1].P2, ' ', SoundCurrentPriority, ' ', Length(SoundBuffer), ' ', deltaX, ' ', deltaY);
	Flush(output);

	ResetScene;
	MessageFakeNotShown := false;
	Board.Tiles[10][10].Element := E_FAKE;
	Board.Tiles[10][10].Color := $2C;
	ElementDefs[E_FAKE].TouchProc(10, 10, 0, deltaX, deltaY);
	WriteLn(output, 'fake-repeat ', Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color, ' ',
		Length(Board.Info.Message), ' ', Ord(MessageFakeNotShown), ' ', Board.StatCount, ' ',
		SoundCurrentPriority, ' ', Length(SoundBuffer), ' ', deltaX, ' ', deltaY);
	Flush(output);

	ResetScene;
	Board.Tiles[10][10].Element := E_WATER;
	Board.Tiles[10][10].Color := $2D;
	ElementDefs[E_WATER].TouchProc(10, 10, 0, deltaX, deltaY);
	WriteLn(output, 'water ', Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color, ' ',
		Length(Board.Info.Message), ' ', Board.StatCount, ' ', Board.Stats[1].P2, ' ',
		SoundCurrentPriority, ' ', Length(SoundBuffer), ' ',
		Ord(SoundBuffer[1]), ' ', Ord(SoundBuffer[2]), ' ', Ord(SoundBuffer[3]), ' ', Ord(SoundBuffer[4]), ' ',
		deltaX, ' ', deltaY);
	Close(output);
	Dispose(IoTmpBuf);
end.
