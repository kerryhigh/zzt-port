program SHORTRD;
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
	ptr: pointer;
	tailByte: byte;
	loadedBytes: array[0..3] of byte;
begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	New(IoTmpBuf);
	WorldCreate;
	World.BoardCount := 1;
	World.BoardLen[1] := 16;
	GetMem(World.BoardData[1], World.BoardLen[1]);
	FillChar(World.BoardData[1]^, World.BoardLen[1], $A5);
	for boardId := 2 to MAX_BOARD do begin
		World.BoardData[boardId] := nil;
		World.BoardLen[boardId] := 0;
	end;
	loaded := WorldLoad('PARTIAL', '.ZZT', false);
	if loaded then begin
		ptr := World.BoardData[1];
		Move(ptr^, loadedBytes, SizeOf(loadedBytes));
		AdvancePointer(ptr, 4);
		Move(ptr^, tailByte, SizeOf(tailByte));
	end else begin
		FillChar(loadedBytes, SizeOf(loadedBytes), 0);
		tailByte := 0;
	end;
	Assign(output, 'SHORTRD.OUT');
	Rewrite(output);
	WriteLn(output, 'short-read ', Ord(loaded), ' ', World.BoardCount, ' ',
		World.Info.CurrentBoard, ' ', Ord(World.Info.IsSave), ' ', World.BoardLen[1], ' ',
		Ord(World.BoardData[1] <> nil), ' data ',
		loadedBytes[0], ' ', loadedBytes[1], ' ', loadedBytes[2], ' ',
		loadedBytes[3], ' tail ', tailByte);
	Close(output);
end.
