program FULLLOAD;
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
	boardId: integer;
begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	New(IoTmpBuf);
	WorldCreate;
	for boardId := 1 to MAX_BOARD do begin
		World.BoardData[boardId] := nil;
		World.BoardLen[boardId] := 0;
	end;
	loaded := WorldLoad('FULLFILE', '.ZZT', false);
	Assign(output, 'FULLLOAD.OUT');
	Rewrite(output);
	if loaded then begin
		if World.BoardLen[0] > 0 then boardPresent := 1 else boardPresent := 0;
		WriteLn(output, 'full-load ', Ord(loaded), ' ', World.BoardCount, ' ',
			World.Info.CurrentBoard, ' ', Ord(World.Info.IsSave), ' ', boardPresent,
			' missing-len ', World.BoardLen[1], ' ', World.BoardLen[15]);
	end else begin
		WriteLn(output, 'full-load 0');
	end;
	Close(output);
end.
