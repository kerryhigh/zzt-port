program BRDALI;
uses Crt, Video, GameVars, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
	brdFile: file;
	boardId, sourceStat, targetStat, statId, otherStat: integer;
	aliasLength, boardLength: integer;
	aliasFound, targetIsUnique: boolean;
begin
	VideoWriteText := QuietWrite;
	New(IoTmpBuf);
	Assign(output, 'BRDALI.OUT');
	Rewrite(output);
	WorldCreate;
	if not WorldLoad('DEMO', '.ZZT', false) then Halt(1);
	{ BoardClose the initially opened board before scanning the saved board records. }
	BoardClose;
	aliasFound := false;
	for boardId := 0 to World.BoardCount do if not aliasFound then begin
		BoardOpen(boardId);
		sourceStat := 0;
		targetStat := 0;
		for statId := 1 to Board.StatCount do
			if (sourceStat = 0) and (Board.Stats[statId].DataLen > 0) and
				(Board.Stats[statId].Data <> nil) then
				sourceStat := statId;

		if sourceStat > 0 then
			for statId := sourceStat + 1 to Board.StatCount do begin
				if targetStat = 0 then begin
					targetIsUnique := true;
					if (Board.Stats[statId].DataLen > 0) and
						(Board.Stats[statId].Data <> nil) then
						for otherStat := 0 to Board.StatCount do
							if (otherStat <> statId) and
								(Board.Stats[otherStat].Data = Board.Stats[statId].Data) then
								targetIsUnique := false;
					if (Board.Stats[statId].DataLen <= 0) or
						(Board.Stats[statId].Data = Board.Stats[sourceStat].Data) or targetIsUnique then
						targetStat := statId;
				end;
			end;

		if (sourceStat > 0) and (targetStat > 0) then begin
			aliasLength := Board.Stats[sourceStat].DataLen;
			if (Board.Stats[targetStat].DataLen > 0) and
				(Board.Stats[targetStat].Data <> nil) and
				(Board.Stats[targetStat].Data <> Board.Stats[sourceStat].Data) then
				FreeMem(Board.Stats[targetStat].Data, Board.Stats[targetStat].DataLen);
			Board.Stats[targetStat].Data := Board.Stats[sourceStat].Data;
			Board.Stats[targetStat].DataLen := aliasLength;
			BoardClose;
			if Board.Stats[targetStat].DataLen >= 0 then Halt(2);

			boardLength := World.BoardLen[boardId];
			Assign(brdFile, 'ALIAS.BRD');
			Rewrite(brdFile, 1);
			BlockWrite(brdFile, boardLength, SizeOf(word));
			BlockWrite(brdFile, World.BoardData[boardId]^, boardLength);
			Close(brdFile);
			WriteLn(output, 'alias ', boardId, ' ', sourceStat, ' ', targetStat,
				' ', aliasLength, ' ', Board.Stats[targetStat].DataLen);
			aliasFound := true;
		end else
			BoardClose;
	end;
	if not aliasFound then Halt(3);
	Close(output);
end.
