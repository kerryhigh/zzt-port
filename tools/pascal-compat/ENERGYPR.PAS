program EnergizerTouchProbe;
uses Crt, Video, GameVars, Elements, Sounds, Oop, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
	debugOutput: text;
	quietX, quietY: integer;
	deltaX, deltaY: integer;
	objectScript: string[255];

procedure Mark(stage: string);
begin
	Assign(debugOutput, 'ENERGYDBG.OUT');
	Rewrite(debugOutput);
	WriteLn(debugOutput, stage);
	Close(debugOutput);
end;

begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	TickTimeDuration := 8;
	New(IoTmpBuf);
	Assign(output, 'ENERGY.OUT');
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
	WriteLn(output, 'energizer-state');
	Flush(output);

	objectScript := '@TARGET'#13':ENERGIZE'#13'#CYCLE 5'#13;
	Board.StatCount := 1;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].P2 := 0;
	Board.Stats[1].Data := Ptr(Seg(objectScript), Ofs(objectScript) + 1);
	Board.Stats[1].DataPos := 0;
	Board.Stats[1].DataLen := Length(objectScript);
	Board.Tiles[10][10].Element := E_OBJECT;
	Board.Tiles[11][10].Element := E_ENERGIZER;
	Board.Tiles[11][10].Color := $3F;
	World.Info.EnergizerTicks := 0;
	MessageEnergizerNotShown := true;
	deltaX := 3;
	deltaY := -2;
	quietX := 0;
	quietY := 0;
	Mark('before-touch');
	ElementDefs[E_ENERGIZER].TouchProc(11, 10, 0, deltaX, deltaY);
	Mark('after-touch');

	Mark('before-output');
	WriteLn(output, 'energizer ',
		Board.Tiles[11][10].Element, ' ', Board.Tiles[11][10].Color, ' ',
		World.Info.EnergizerTicks, ' ', SoundCurrentPriority, ' ', Length(SoundBuffer), ' ',
		Ord(SoundBuffer[1]), ' ', Ord(SoundBuffer[2]), ' ',
		Ord(SoundBuffer[Length(SoundBuffer)-1]), ' ', Ord(SoundBuffer[Length(SoundBuffer)]), ' ',
		Length(Board.Info.Message), ' ', Ord(MessageEnergizerNotShown), ' ', Board.StatCount, ' ',
		Board.Stats[2].X, ' ', Board.Stats[2].Y, ' ', Board.Stats[2].P2, ' ', Board.Stats[1].DataPos, ' ',
	deltaX, ' ', deltaY);
	Close(output);
	Dispose(IoTmpBuf);
end.
