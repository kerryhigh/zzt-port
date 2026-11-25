program ObjectTouchProbe;
uses Crt, Video, GameVars, Elements, Game;

{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}

var
	caseId, targetId, sourceId, deltaX, deltaY: integer;
	script: string[255];
	output: text;

procedure SetObject(statId, x, y: integer);
begin
	{ Stat 2 is inactive in cases 1-3; seed its printed DataPos. }
	{ Case 4 sets its active DataPos explicitly. }
	if Board.StatCount < 2 then
		Board.Stats[2].DataPos := 0;
	Board.Stats[statId].X := x;
	Board.Stats[statId].Y := y;
	Board.Stats[statId].Cycle := 3;
	Board.Stats[statId].P2 := 0;
	Board.Stats[statId].DataPos := 2;
	Board.Tiles[x][y].Element := E_OBJECT;
	Board.Tiles[x][y].Color := $2A;
	GetMem(Board.Stats[statId].Data, Length(script));
	Move(script[1], Board.Stats[statId].Data^, Length(script));
	Board.Stats[statId].DataLen := Length(script);
end;

procedure RecordCase;
begin
	ElementDefs[E_OBJECT].TouchProc(
		Board.Stats[targetId].X, Board.Stats[targetId].Y,
		sourceId, deltaX, deltaY);
	WriteLn(output, caseId, ' ', targetId,
		' ', Board.Stats[1].DataPos,
		' ', Board.Stats[2].DataPos,
		' ', Board.Stats[targetId].Cycle,
		' ', Board.Stats[targetId].P2,
		' ', deltaX, ' ', deltaY);
	Flush(output);
end;

begin
	VideoWriteText := QuietWrite;
	New(IoTmpBuf);
	Assign(output, 'OBJTOUCH.OUT');
	Rewrite(output);

	caseId := 1;
	WorldCreate;
	BoardCreate;
	Board.StatCount := 1;
	script := '#CYCLE 3' + #13 + '#END' + #13 + ':TOUCH' + #13 + '#CYCLE 9' + #13 + '#END' + #13;
	SetObject(1, 10, 10);
	targetId := 1;
	sourceId := 0;
	deltaX := 6;
	deltaY := -4;
	RecordCase;
	WorldUnload;

	caseId := 2;
	WorldCreate;
	BoardCreate;
	Board.StatCount := 1;
	script := '#CYCLE 3' + #13 + '#END' + #13;
	SetObject(1, 10, 10);
	targetId := 1;
	sourceId := 0;
	deltaX := 6;
	deltaY := -4;
	RecordCase;
	WorldUnload;

	caseId := 3;
	WorldCreate;
	BoardCreate;
	Board.StatCount := 1;
	script := '#CYCLE 3' + #13 + '#END' + #13 + ':TOUCH' + #13 + '#CYCLE 9' + #13 + '#END' + #13;
	SetObject(1, 10, 10);
	Board.Stats[1].P2 := 1;
	targetId := 1;
	sourceId := 0;
	deltaX := 6;
	deltaY := -4;
	RecordCase;
	WorldUnload;

	caseId := 4;
	WorldCreate;
	BoardCreate;
	Board.StatCount := 2;
	script := '#CYCLE 4' + #13 + '#END' + #13 + ':TOUCH' + #13 + '#CYCLE 6' + #13 + '#END' + #13;
	SetObject(1, 8, 8);
	script := '#CYCLE 3' + #13 + '#END' + #13 + ':TOUCH' + #13 + '#CYCLE 9' + #13 + '#END' + #13;
	SetObject(2, 10, 10);
	Board.Stats[1].DataPos := 3;
	Board.Stats[2].DataPos := 4;
	targetId := 2;
	sourceId := 1;
	deltaX := 6;
	deltaY := -4;
	RecordCase;
	WorldUnload;

	Close(output);
	Dispose(IoTmpBuf);
end.
