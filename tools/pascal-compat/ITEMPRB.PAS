program ItemTouchProbe;
uses Crt, Video, GameVars, Elements, Sounds, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
	deltaX, deltaY: integer;

begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	TickTimeDuration := 4;
	Assign(output, 'ITEMPRB.OUT');
	Rewrite(output);

	New(IoTmpBuf);
	WorldCreate;
	GameStateElement := E_PLAYER;
	DebugEnabled := false;
	SoundEnabled := false;
	SoundBlockQueueing := false;
	SoundCurrentPriority := -1;
	SoundIsPlaying := false;
	SoundBuffer := '';
	WriteLn(output, 'player-state');
	Flush(output);
	World.Info.Ammo := 7;
	MessageAmmoNotShown := false;
	Board.Tiles[10][10].Element := E_AMMO;
	deltaX := 0;
	deltaY := 0;
	ElementDefs[E_AMMO].TouchProc(10, 10, 0, deltaX, deltaY);
	if World.Info.Ammo <> 12 then Halt(1);
	if Board.Tiles[10][10].Element <> E_EMPTY then Halt(2);
	WriteLn(output, 'ammo ', World.Info.Ammo, ' ', Board.Tiles[10][10].Element);
	Flush(output);

	BoardCreate;
	World.Info.Ammo := 32765;
	MessageAmmoNotShown := false;
	Board.Tiles[10][10].Element := E_AMMO;
	ElementDefs[E_AMMO].TouchProc(10, 10, 0, deltaX, deltaY);
	if World.Info.Ammo <> -32766 then Halt(3);
	if Board.Tiles[10][10].Element <> E_EMPTY then Halt(4);
	WriteLn(output, 'ammo-overflow ', World.Info.Ammo, ' ', Board.Tiles[10][10].Element);
	Flush(output);

	BoardCreate;
	World.Info.Gems := 7;
	World.Info.Health := 100;
	World.Info.Score := 50;
	MessageGemNotShown := false;
	Board.Tiles[10][10].Element := E_GEM;
	ElementDefs[E_GEM].TouchProc(10, 10, 0, deltaX, deltaY);
	if World.Info.Gems <> 8 then Halt(5);
	if World.Info.Health <> 101 then Halt(6);
	if World.Info.Score <> 60 then Halt(7);
	if Board.Tiles[10][10].Element <> E_EMPTY then Halt(8);
	WriteLn(output, 'gem ', World.Info.Gems, ' ', World.Info.Health, ' ',
		World.Info.Score, ' ', Board.Tiles[10][10].Element);
	Flush(output);

	BoardCreate;
	World.Info.Gems := 32767;
	World.Info.Health := -2;
	World.Info.Score := 32760;
	MessageGemNotShown := false;
	Board.Tiles[10][10].Element := E_GEM;
	ElementDefs[E_GEM].TouchProc(10, 10, 0, deltaX, deltaY);
	if World.Info.Gems <> -32768 then Halt(9);
	if World.Info.Health <> 0 then Halt(10);
	if World.Info.Score <> -32766 then Halt(11);
	if Board.Tiles[10][10].Element <> E_EMPTY then Halt(12);
	WriteLn(output, 'gem-overflow ', World.Info.Gems, ' ', World.Info.Health, ' ',
		World.Info.Score, ' ', Board.Tiles[10][10].Element);
	Flush(output);

	BoardCreate;
	World.Info.Torches := 5;
	MessageTorchNotShown := false;
	Board.Tiles[10][10].Element := E_TORCH;
	ElementDefs[E_TORCH].TouchProc(10, 10, 0, deltaX, deltaY);
	if World.Info.Torches <> 6 then Halt(13);
	if Board.Tiles[10][10].Element <> E_EMPTY then Halt(14);
	if MessageTorchNotShown then Halt(15);
	WriteLn(output, 'torch ', World.Info.Torches, ' ', Board.Tiles[10][10].Element, ' ',
		Ord(MessageTorchNotShown));
	Flush(output);

	Close(output);
	Dispose(IoTmpBuf);
end.
