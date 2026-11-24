program BOARDIN;
uses Crt, Video, GameVars, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
	hintValue: integer;
begin
	New(IoTmpBuf);
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	TickTimeDuration := 8;
	GameStateElement := E_PLAYER;
	Assign(output, 'BOARDIN.OUT');
	Rewrite(output);
	WorldCreate;

	BoardCreate;
	MessageHintTorchNotShown := true;
	Board.Stats[0].X := 12;
	Board.Stats[0].Y := 19;
	Board.Info.StartPlayerX := 3;
	Board.Info.StartPlayerY := 4;
	World.Info.BoardTimeSec := 37;
	Flush(output);
	BoardEnter;
	if MessageHintTorchNotShown then hintValue := 1 else hintValue := 0;
	WriteLn(output, 'light start ', Board.Info.StartPlayerX, ' ', Board.Info.StartPlayerY,
		' time ', World.Info.BoardTimeSec, ' hint ', hintValue,
		' stats ', Board.StatCount, ' message-length ', Length(Board.Info.Message));
	Flush(output);

	BoardCreate;
	MessageHintTorchNotShown := true;
	Board.Info.IsDark := true;
	Board.Stats[0].X := 13;
	Board.Stats[0].Y := 17;
	Board.Info.StartPlayerX := 3;
	Board.Info.StartPlayerY := 4;
	World.Info.BoardTimeSec := 53;
	BoardEnter;
	if MessageHintTorchNotShown then hintValue := 1 else hintValue := 0;
	WriteLn(output, 'dark-first start ', Board.Info.StartPlayerX, ' ', Board.Info.StartPlayerY,
		' time ', World.Info.BoardTimeSec, ' hint ', hintValue,
		' stats ', Board.StatCount, ' timer ', Board.Stats[Board.StatCount].P2,
		' message ', Board.Info.Message);
	Flush(output);

	Board.Stats[0].X := 20;
	Board.Stats[0].Y := 12;
	World.Info.BoardTimeSec := 24;
	BoardEnter;
	if MessageHintTorchNotShown then hintValue := 1 else hintValue := 0;
	WriteLn(output, 'dark-repeat start ', Board.Info.StartPlayerX, ' ', Board.Info.StartPlayerY,
		' time ', World.Info.BoardTimeSec, ' hint ', hintValue,
		' stats ', Board.StatCount, ' timer ', Board.Stats[Board.StatCount].P2,
		' message-length ', Length(Board.Info.Message));
	Flush(output);

	Close(output);
end.
