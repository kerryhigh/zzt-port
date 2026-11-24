program WORLDTIT;
uses Crt, Video, GameVars, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
	loaded: boolean;
	boardPresent: byte;
begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	New(IoTmpBuf);
	Assign(output, 'WORLDTIT.OUT');
	Rewrite(output);
	WorldCreate;
	loaded := WorldLoad('TITLE', '.ZZT', true);
	if not loaded then begin
		WriteLn(output, 'title-only failed');
		Close(output);
		Halt(1);
	end;
	if World.BoardLen[0] > 0 then boardPresent := 1 else boardPresent := 0;
	WriteLn(output, 'title-only ', Ord(loaded), ' ', World.BoardCount, ' ',
		World.Info.CurrentBoard, ' ', Ord(World.Info.IsSave), ' ', boardPresent);
	Close(output);
end.
