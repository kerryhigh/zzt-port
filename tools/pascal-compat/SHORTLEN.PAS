program SHORTLEN;
uses Crt, Video, GameVars, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
	loaded: boolean;
	boardId: integer;
begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	New(IoTmpBuf);
	WorldCreate;
	World.BoardCount := 1;
	World.BoardLen[1] := $0100;
	GetMem(World.BoardData[1], World.BoardLen[1]);
	FillChar(World.BoardData[1]^, World.BoardLen[1], $5A);
	for boardId := 2 to MAX_BOARD do begin
		World.BoardData[boardId] := nil;
		World.BoardLen[boardId] := 0;
	end;
	loaded := WorldLoad('PARTLEN', '.ZZT', false);
	Assign(output, 'SHORTLEN.OUT');
	Rewrite(output);
	WriteLn(output, 'short-len ', Ord(loaded), ' ', World.BoardCount, ' ',
		World.Info.CurrentBoard, ' ', World.BoardLen[1]);
	Close(output);
end.
