program ScrollTickProbe;
uses Crt, Video, GameVars, Elements, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
	colorValue: integer;
begin
	New(IoTmpBuf);
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	TickTimeDuration := TickSpeed * 2;
	WorldCreate;
	BoardCreate;
	Board.StatCount := 1;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Tiles[10][10].Element := E_SCROLL;
	Assign(output, 'SCROLLTK.OUT');
	Rewrite(output);
	for colorValue := 0 to 255 do begin
		Board.Tiles[10][10].Color := colorValue;
		ElementDefs[E_SCROLL].TickProc(1);
		WriteLn(output, colorValue, ' ', Board.Tiles[10][10].Color);
	end;
	Close(output);
end.
