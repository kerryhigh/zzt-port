program OopCharProbe;
uses Crt, Video, GameVars, Oop, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
const
	caseCount = 8;
var
	caseId, errorCode, position: integer;
	script: string[255];
	outputFile: TString50;
	output: text;

begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	Val(ParamStr(1), caseId, errorCode);
	if (errorCode <> 0) or (caseId < 1) or (caseId > caseCount) then
		Halt(1);
	New(IoTmpBuf);
	WorldCreate;
	BoardCreate;
	Board.StatCount := 1;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].Cycle := 3;
	Board.Stats[1].P1 := $A5;
	Board.Stats[1].DataPos := 0;
	Board.Tiles[10][10].Element := E_OBJECT;
	Board.Tiles[10][10].Color := $0F;
	case caseId of
		1: script := '#CHAR -32768 #CYCLE 5' + #13;
		2: script := '#CHAR -1 #CYCLE 5' + #13;
		3: script := '#CHAR 0 #CYCLE 5' + #13;
		4: script := '#CHAR 1 #CYCLE 5' + #13;
		5: script := '#CHAR 127 #CYCLE 5' + #13;
		6: script := '#CHAR 255 #CYCLE 5' + #13;
		7: script := '#CHAR 256 #CYCLE 5' + #13;
		8: script := '#CHAR 32767 #CYCLE 5' + #13;
	end;
	GetMem(Board.Stats[1].Data, Length(script));
	Move(script[1], Board.Stats[1].Data^, Length(script));
	Board.Stats[1].DataLen := Length(script);
	position := 0;
	OopExecute(1, position, 'Char probe');
	outputFile := 'OOPCH00.OUT';
	outputFile[6] := Chr(Ord('0') + (caseId div 10));
	outputFile[7] := Chr(Ord('0') + (caseId mod 10));
	Assign(output, outputFile);
	Rewrite(output);
	WriteLn(output, caseId, ' ', Board.Stats[1].P1, ' ',
		Board.Stats[1].Cycle, ' ', position);
	Close(output);
end.
