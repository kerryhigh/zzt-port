program BulletTickProbe;
uses Crt, Video, GameVars, Elements, Sounds, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
	i: integer;

procedure ResetBullet(targetElement, targetColor: byte);
begin
	BoardCreate;
	Board.StatCount := 1;
	Board.Stats[0].X := 30;
	Board.Stats[0].Y := 12;
	Board.Tiles[30][12].Element := E_PLAYER;
	Board.Tiles[30][12].Color := ElementDefs[E_PLAYER].Color;
	Board.Stats[1] := StatTemplateDefault;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].StepX := 1;
	Board.Stats[1].StepY := 0;
	Board.Stats[1].P1 := 1;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := $70;
	Board.Tiles[10][10].Element := E_BULLET;
	Board.Tiles[10][10].Color := $2E;
	Board.Tiles[11][10].Element := targetElement;
	Board.Tiles[11][10].Color := targetColor;
	Board.Tiles[9][10].Element := E_EMPTY;
	Board.Tiles[9][10].Color := $70;
	Board.Tiles[10][9].Element := E_EMPTY;
	Board.Tiles[10][9].Color := $70;
	Board.Tiles[10][11].Element := E_EMPTY;
	Board.Tiles[10][11].Color := $70;
	CurrentStatTicked := 1;
	SoundCurrentPriority := -1;
	SoundIsPlaying := false;
	SoundBuffer := '';
end;

procedure PrintState(labelText: string);
begin
	Write(output, labelText,
		' stats ', Board.StatCount,
		' bullet ', Board.Stats[1].X, ' ', Board.Stats[1].Y,
		' step ', Board.Stats[1].StepX, ' ', Board.Stats[1].StepY,
		' old ', Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color,
		' target ', Board.Tiles[11][10].Element, ' ', Board.Tiles[11][10].Color,
		' left ', Board.Tiles[9][10].Element, ' ', Board.Tiles[9][10].Color,
		' up ', Board.Tiles[10][9].Element, ' ', Board.Tiles[10][9].Color,
		' down ', Board.Tiles[10][11].Element, ' ', Board.Tiles[10][11].Color,
		' under ', Board.Stats[1].Under.Element, ' ', Board.Stats[1].Under.Color,
		' tick ', CurrentStatTicked,
		' sound ', SoundCurrentPriority, ' ', Length(SoundBuffer));
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
	Assign(output, 'BULLETPR.OUT');
	Rewrite(output);
	WorldCreate;
	SoundEnabled := false;
	SoundBlockQueueing := false;

	ResetBullet(E_EMPTY, $70);
	ElementDefs[E_BULLET].TickProc(1);
	PrintState('bullet-move');

	ResetBullet(E_WATER, $4D);
	ElementDefs[E_BULLET].TickProc(1);
	PrintState('bullet-water');

	ResetBullet(E_RICOCHET, $0F);
	ElementDefs[E_BULLET].TickProc(1);
	PrintState('bullet-direct-ricochet');

	ResetBullet(E_SOLID, $0F);
	Board.Tiles[10][11].Element := E_RICOCHET;
	Board.Tiles[10][11].Color := $0F;
	ElementDefs[E_BULLET].TickProc(1);
	PrintState('bullet-side-down');

	ResetBullet(E_SOLID, $0F);
	Board.Tiles[10][9].Element := E_RICOCHET;
	Board.Tiles[10][9].Color := $0F;
	ElementDefs[E_BULLET].TickProc(1);
	PrintState('bullet-side-up');

	ResetBullet(E_SOLID, $0F);
	ElementDefs[E_BULLET].TickProc(1);
	PrintState('bullet-blocked');

	Close(output);
	WorldUnload;
	Dispose(IoTmpBuf);
end.
