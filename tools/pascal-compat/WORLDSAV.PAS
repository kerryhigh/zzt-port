program WorldSaveProbe;
uses Crt, Dos, Video, Keys, Sounds, Input, TxtWind, GameVars, Elements, Editor, Oop, Game;
var
	signedValues: array[0..4] of integer;
	signedNames: array[0..4] of string[8];
	boardCounts: array[0..3] of integer;
	boardNames: array[0..3] of string[8];
	longText: string[58];
	tag, i, boardId, sourceBoardCount: integer;
	rawValue: byte;
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

	{ Keep the active board valid while exercising every signed world-save
	  value in one direct Pascal session. }
	signedValues[0] := -32768;
	signedValues[1] := -1;
	signedValues[2] := 0;
	signedValues[3] := 1;
	signedValues[4] := 32767;
	signedNames[0] := 'WRS0';
	signedNames[1] := 'WRS1';
	signedNames[2] := 'WRS2';
	signedNames[3] := 'WRS3';
	signedNames[4] := 'WRS4';
	for tag := 0 to 4 do begin
		World.Info.Ammo := signedValues[tag];
		World.Info.Gems := signedValues[tag];
		World.Info.Health := signedValues[tag];
		World.Info.Torches := signedValues[tag];
		World.Info.TorchTicks := signedValues[tag];
		World.Info.EnergizerTicks := signedValues[tag];
		World.Info.Score := signedValues[tag];
		World.Info.BoardTimeSec := signedValues[tag];
		World.Info.BoardTimeHsec := signedValues[tag];
		WorldSave(signedNames[tag], '.SAV');
	end;

	{ Preserve raw Boolean representations and the 14-byte WorldInfo tail. }
	signedValues[0] := 0;
	signedValues[1] := 1;
	signedValues[2] := 2;
	signedValues[3] := 255;
	signedNames[0] := 'BLS0';
	signedNames[1] := 'BLS1';
	signedNames[2] := 'BLS2';
	signedNames[3] := 'BLS3';
	for tag := 0 to 3 do begin
		rawValue := signedValues[tag];
		for i := 1 to 7 do
			Move(rawValue, World.Info.Keys[i], SizeOf(rawValue));
		Move(rawValue, World.Info.IsSave, SizeOf(rawValue));
		for i := 0 to 13 do
			World.Info.unkPad[i] := signedValues[i mod 4];
		WorldSave(signedNames[tag], '.SAV');
	end;

	{ Short-string assignments keep bytes from the previous longer value. }
	longText := '';
	for i := 1 to 58 do
		longText := longText + Chr(Ord('A') + ((i - 1) mod 26));
	World.Info.Name := Copy(longText, 1, 20);
	World.Info.Name := 'N';
	World.Info.Flags[1] := Copy(longText, 1, 20);
	World.Info.Flags[1] := 'F';
	Board.Name := Copy(longText, 1, 50);
	Board.Name := 'B';
	Board.Info.Message := longText;
	Board.Info.Message := 'M';
	for i := 1 to 7 do
		World.Info.Keys[i] := false;
	World.Info.IsSave := true;
	WorldSave('SAVTAIL', '.SAV');
	World.Info.IsSave := false;
	WorldSave('ZZTTAIL', '.ZZT');

	{ Preserve the frozen world's board records while testing WorldSave's
	  documented 0/1/MAX_BOARD-1/MAX_BOARD count boundaries. Each slot gets
	  its own copy because WorldSave frees and replaces the active record. }
	BoardClose;
	BoardOpen(0);
	sourceBoardCount := World.BoardCount;
	for boardId := 1 to MAX_BOARD do begin
		if boardId <= sourceBoardCount then
			FreeMem(World.BoardData[boardId], World.BoardLen[boardId]);
		GetMem(World.BoardData[boardId], World.BoardLen[0]);
		Move(World.BoardData[0]^, World.BoardData[boardId]^, World.BoardLen[0]);
		World.BoardLen[boardId] := World.BoardLen[0];
	end;
	boardCounts[0] := 0;
	boardCounts[1] := 1;
	boardCounts[2] := MAX_BOARD - 1;
	boardCounts[3] := MAX_BOARD;
	boardNames[0] := 'BCNT0';
	boardNames[1] := 'BCNT1';
	boardNames[2] := 'BCNT99';
	boardNames[3] := 'BCNT100';
	for tag := 0 to 3 do begin
		World.BoardCount := boardCounts[tag];
		World.Info.CurrentBoard := 0;
		for i := 1 to 7 do
			World.Info.Keys[i] := false;
		for i := 0 to 13 do
			World.Info.unkPad[i] := 0;
		World.Info.IsSave := true;
		WorldSave(boardNames[tag], '.SAV');
		World.Info.IsSave := false;
		WorldSave(boardNames[tag], '.ZZT');
	end;
end.
