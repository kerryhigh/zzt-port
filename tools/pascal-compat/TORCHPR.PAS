program TorchTouchProbe;
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
	Assign(debugOutput, 'TORCHDB.OUT');
	Rewrite(debugOutput);
	WriteLn(debugOutput, stage);
	Close(debugOutput);
end;

begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	TickTimeDuration := 4;
	New(IoTmpBuf);
	Assign(output, 'TORCH.OUT');
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
	WriteLn(output, 'torch-state');
	Flush(output);

	World.Info.Torches := 32767;
	MessageTorchNotShown := true;
	Board.Tiles[10][10].Element := E_TORCH;
	Board.Tiles[10][10].Color := $2E;
	deltaX := 2;
	deltaY := -1;
	ElementDefs[E_TORCH].TouchProc(10, 10, 0, deltaX, deltaY);
	if World.Info.Torches <> -32768 then Halt(1);
	if Board.Tiles[10][10].Element <> E_EMPTY then Halt(2);
	if Board.Tiles[10][10].Color <> $2E then Halt(3);
	if Board.Info.Message <> 'Torch - used for lighting in the underground.' then Halt(4);
	if MessageTorchNotShown then Halt(5);
	if Board.StatCount <> 1 then Halt(6);
	if (Board.Stats[1].X <> 0) or (Board.Stats[1].Y <> 0) then Halt(7);
	if Board.Stats[1].P2 <> 40 then Halt(8);
	if (SoundCurrentPriority <> 3) or (Length(SoundBuffer) <> 6) then Halt(9);
	if (Ord(SoundBuffer[1]) <> 48) or (Ord(SoundBuffer[2]) <> 1)
		or (Ord(SoundBuffer[3]) <> 57) or (Ord(SoundBuffer[4]) <> 1)
		or (Ord(SoundBuffer[5]) <> 52) or (Ord(SoundBuffer[6]) <> 2) then Halt(10);
	WriteLn(output, 'torch-message ', World.Info.Torches, ' ',
		Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color, ' ',
		Length(Board.Info.Message), ' ', Ord(MessageTorchNotShown), ' ',
		Board.StatCount, ' ', Board.Stats[1].X, ' ', Board.Stats[1].Y, ' ', Board.Stats[1].P2, ' ',
		SoundCurrentPriority, ' ', Length(SoundBuffer), ' ',
		Ord(SoundBuffer[1]), ' ', Ord(SoundBuffer[2]), ' ',
		Ord(SoundBuffer[3]), ' ', Ord(SoundBuffer[4]), ' ',
		Ord(SoundBuffer[5]), ' ', Ord(SoundBuffer[6]), ' ', deltaX, ' ', deltaY);

	BoardCreate;
	World.Info.Torches := 0;
	MessageTorchNotShown := false;
	SoundCurrentPriority := -1;
	SoundIsPlaying := false;
	SoundBuffer := '';
	Board.Tiles[10][10].Element := E_TORCH;
	Board.Tiles[10][10].Color := $2E;
	deltaX := 2;
	deltaY := -1;
	ElementDefs[E_TORCH].TouchProc(10, 10, 0, deltaX, deltaY);
	if World.Info.Torches <> 1 then Halt(11);
	if Board.Tiles[10][10].Element <> E_EMPTY then Halt(12);
	if Board.Tiles[10][10].Color <> $2E then Halt(13);
	if Length(Board.Info.Message) <> 0 then Halt(14);
	if MessageTorchNotShown then Halt(15);
	if Board.StatCount <> 0 then Halt(16);
	if (SoundCurrentPriority <> 3) or (Length(SoundBuffer) <> 6) then Halt(17);
	WriteLn(output, 'torch-silent ', World.Info.Torches, ' ',
		Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color, ' ',
		Length(Board.Info.Message), ' ', Ord(MessageTorchNotShown), ' ', Board.StatCount, ' ',
		SoundCurrentPriority, ' ', Length(SoundBuffer), ' ',
		Ord(SoundBuffer[1]), ' ', Ord(SoundBuffer[2]), ' ',
		Ord(SoundBuffer[3]), ' ', Ord(SoundBuffer[4]), ' ',
		Ord(SoundBuffer[5]), ' ', Ord(SoundBuffer[6]), ' ', deltaX, ' ', deltaY);
	Close(output);
	Dispose(IoTmpBuf);
end.
