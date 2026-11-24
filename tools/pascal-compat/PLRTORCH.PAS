program PlayerTorchProbe;
uses Crt, Video, GameVars, Elements, Input, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
begin
	VideoWriteText := QuietWrite;
	New(IoTmpBuf);
	TickSpeed := 4;
	WorldCreate;
	PlayerDirX := 0;
	PlayerDirY := 0;
	Board.Info.IsDark := true;
	World.Info.Torches := 1;
	InputDeltaX := 0;
	InputDeltaY := 0;
	InputShiftPressed := false;
	InputKeyPressed := 'T';
	ElementDefs[E_PLAYER].TickProc(0);
	Assign(output, 'PLRTORCH.OUT');
	Rewrite(output);
	WriteLn(output, World.Info.Torches, ' ', World.Info.TorchTicks, ' ', Board.Stats[0].X, ' ', Board.Stats[0].Y);
	Close(output);
	Dispose(IoTmpBuf);
end.
