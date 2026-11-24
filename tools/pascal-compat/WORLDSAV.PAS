program WorldSaveProbe;
uses Crt, Dos, Video, Keys, Sounds, Input, TxtWind, GameVars, Elements, Editor, Oop, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
begin
	VideoWriteText := QuietWrite;
	New(IoTmpBuf);
	TickSpeed := 4;
	WorldCreate;
	if not WorldLoad('DEMO', '.ZZT', false) then Halt(1);
	World.Info.IsSave := true;
	WorldSave('PASOUT', '.SAV');
end.
