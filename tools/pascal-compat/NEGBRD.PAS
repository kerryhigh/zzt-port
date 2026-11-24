program NEGBRD;
uses Crt, Video, GameVars, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	f: file;
	output: text;
begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	New(IoTmpBuf);
	WorldCreate;
	Assign(f, 'NEGBRD.BRD');
	Reset(f, 1);
	BoardClose;
	FreeMem(World.BoardData[World.Info.CurrentBoard], World.BoardLen[World.Info.CurrentBoard]);
	BlockRead(f, World.BoardLen[World.Info.CurrentBoard], 2);
	GetMem(World.BoardData[World.Info.CurrentBoard], World.BoardLen[World.Info.CurrentBoard]);
	BlockRead(f, World.BoardData[World.Info.CurrentBoard]^, World.BoardLen[World.Info.CurrentBoard]);
	Close(f);
	BoardOpen(World.Info.CurrentBoard);
	Assign(output, 'NEGBRD.OUT');
	Rewrite(output);
	WriteLn(output, 'negative-brd 1 ', World.BoardLen[World.Info.CurrentBoard]);
	Close(output);
end.
