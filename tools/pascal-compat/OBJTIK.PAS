program ObjectTickProbe;
uses Crt, Video, GameVars, Elements, Game;
var
	caseId, errorCode: integer;
	script: string[255];
	outputFile: TString50;
	output: text;

{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}

begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	Val(ParamStr(1), caseId, errorCode);
	if (errorCode <> 0) or (caseId < 1) or (caseId > 4) then
		Halt(1);
	case caseId of
		1: script := '#CYCLE 7' + #13 + '#END' + #13;
		2: script := '#WALK EAST' + #13 + '#IDLE' + #13;
		3: script := '#WALK EAST' + #13 + '#IDLE' + #13 + ':THUD' + #13 + '#CYCLE 9' + #13 + '#END' + #13;
	else script := '#END' + #13;
	end;
	New(IoTmpBuf);
	WorldCreate;
	BoardCreate;
	Board.StatCount := 1;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].Cycle := 0;
	Board.Stats[1].StepX := 0;
	Board.Stats[1].StepY := 0;
	Board.Stats[1].DataPos := 0;
	Board.Tiles[10][10].Element := E_OBJECT;
	Board.Tiles[10][10].Color := $0A;
	Board.Tiles[11][10].Element := E_EMPTY;
	Board.Tiles[11][10].Color := $31;
	if caseId = 3 then begin
		Board.Tiles[11][10].Element := E_SOLID;
		Board.Tiles[11][10].Color := $1F;
	end;
	if caseId = 4 then begin
		Board.Stats[1].StepX := 1;
		Board.Stats[1].DataPos := -1;
	end;
	GetMem(Board.Stats[1].Data, Length(script));
	Move(script[1], Board.Stats[1].Data^, Length(script));
	Board.Stats[1].DataLen := Length(script);
	ElementDefs[E_OBJECT].TickProc(1);
	outputFile := 'OBJTIK00.OUT';
	outputFile[7] := Chr(Ord('0') + (caseId div 10));
	outputFile[8] := Chr(Ord('0') + (caseId mod 10));
	Assign(output, outputFile);
	Rewrite(output);
	WriteLn(output, caseId, ' ', Board.Stats[1].X, ' ', Board.Stats[1].Y,
		' ', Board.Stats[1].Cycle, ' ', Board.Stats[1].StepX, ' ', Board.Stats[1].StepY,
		' ', Board.Stats[1].DataPos, ' ', Board.Tiles[10][10].Element,
		' ', Board.Tiles[11][10].Element);
	Close(output);
end.
