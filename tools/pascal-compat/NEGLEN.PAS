program NEGLEN;
uses Crt, Video, GameVars, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
	loaded: boolean;
begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	New(IoTmpBuf);
	WorldCreate;
	loaded := WorldLoad('NEGLEN', '.ZZT', false);
	Assign(output, 'NEGLEN.OUT');
	Rewrite(output);
	WriteLn(output, 'negative-len ', Ord(loaded), ' ', World.BoardCount, ' ',
		World.Info.CurrentBoard, ' ', World.BoardLen[0]);
	Close(output);
end.
