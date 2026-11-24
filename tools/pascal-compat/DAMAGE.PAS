program DamageProbe;
uses Crt, Video, GameVars, Elements, Sounds, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
	deltaX, deltaY: integer;

procedure ResetScene;
begin
	WorldCreate;
	BoardCreate;
	GameStateElement := E_PLAYER;
	DebugEnabled := false;
	SoundEnabled := false;
	SoundBlockQueueing := false;
	SoundCurrentPriority := -1;
	SoundIsPlaying := false;
	SoundBuffer := '';
	Board.Stats[0].X := 30;
	Board.Stats[0].Y := 12;
	Board.Tiles[30][12].Element := E_PLAYER;
	Board.Tiles[30][12].Color := $1F;
	deltaX := 2;
	deltaY := -1;
end;

procedure PrintState(name: string);
var
	i: integer;
begin
	Write(output, name, ' health ', World.Info.Health, ' player ', Board.Tiles[30][12].Element, ' ', Board.Tiles[30][12].Color,
		' tile ', Board.Tiles[11][10].Element, ' ', Board.Tiles[11][10].Color,
		' message "', Board.Info.Message, '" stats ', Board.StatCount);
	if Board.StatCount > 0 then
		Write(output, ' ', Board.Stats[1].P2);
	Write(output, ' sound ', SoundCurrentPriority, ' ', Length(SoundBuffer));
	for i := 1 to Length(SoundBuffer) do
		Write(output, ' ', Ord(SoundBuffer[i]));
	WriteLn(output, ' delta ', deltaX, ' ', deltaY);
	Flush(output);
end;

begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	TickTimeDuration := 4;
	New(IoTmpBuf);
	Assign(output, 'DAMAGE.OUT');
	Rewrite(output);

	ResetScene;
	World.Info.Health := 15;
	Board.Info.ReenterWhenZapped := false;
	DamageStat(0);
	PrintState('damage-nonlethal');

	ResetScene;
	World.Info.Health := 5;
	Board.Info.ReenterWhenZapped := false;
	DamageStat(0);
	PrintState('damage-lethal');

	ResetScene;
	Board.StatCount := 1;
	Board.Stats[1].X := 11;
	Board.Stats[1].Y := 10;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := $70;
	Board.Tiles[11][10].Element := E_SLIME;
	Board.Tiles[11][10].Color := $3A;
	ElementDefs[E_SLIME].TouchProc(11, 10, 0, deltaX, deltaY);
	PrintState('slime-touch');

	Close(output);
	Dispose(IoTmpBuf);
end.
