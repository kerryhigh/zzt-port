program OopPutProbe;
uses Crt, Video, GameVars, Oop, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
const caseCount = 8;
var
	caseId, errorCode, position: integer;
	sourceX, sourceY, deltaX, deltaY, targetX, targetY, observedX, observedY: integer;
	direction: string[5];
	script: string[255];
	outputFile: TString50;
	output: text;

begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	Val(ParamStr(1), caseId, errorCode);
	if (errorCode <> 0) or (caseId < 1) or (caseId > caseCount) then
		Halt(1);
	case caseId of
		1: begin sourceX := 10; sourceY := 10; direction := 'NORTH'; deltaX := 0; deltaY := -1; end;
		2: begin sourceX := 10; sourceY := 1; direction := 'NORTH'; deltaX := 0; deltaY := -1; end;
		3: begin sourceX := 10; sourceY := 23; direction := 'SOUTH'; deltaX := 0; deltaY := 1; end;
		4: begin sourceX := 10; sourceY := 24; direction := 'SOUTH'; deltaX := 0; deltaY := 1; end;
		5: begin sourceX := 59; sourceY := 10; direction := 'EAST'; deltaX := 1; deltaY := 0; end;
		6: begin sourceX := 60; sourceY := 10; direction := 'EAST'; deltaX := 1; deltaY := 0; end;
		7: begin sourceX := 1; sourceY := 10; direction := 'WEST'; deltaX := -1; deltaY := 0; end;
		8: begin sourceX := 2; sourceY := 10; direction := 'WEST'; deltaX := -1; deltaY := 0; end;
	end;
	targetX := sourceX + deltaX;
	targetY := sourceY + deltaY;
	observedX := sourceX;
	observedY := sourceY;
	New(IoTmpBuf);
	WorldCreate;
	BoardCreate;
	Board.StatCount := 1;
	Board.Stats[1].X := sourceX;
	Board.Stats[1].Y := sourceY;
	Board.Stats[1].Cycle := 3;
	Board.Stats[1].DataPos := 0;
	Board.Tiles[sourceX][sourceY].Element := E_OBJECT;
	Board.Tiles[sourceX][sourceY].Color := $0F;
	if (targetX >= 0) and (targetX <= BOARD_WIDTH)
		and (targetY >= 0) and (targetY < BOARD_HEIGHT) then begin
		observedX := targetX;
		observedY := targetY;
		Board.Tiles[targetX][targetY].Element := E_EMPTY;
		Board.Tiles[targetX][targetY].Color := $2A;
	end;
	script := '#PUT ' + direction + ' SOLID' + #13 + '#END' + #13;
	GetMem(Board.Stats[1].Data, Length(script));
	Move(script[1], Board.Stats[1].Data^, Length(script));
	Board.Stats[1].DataLen := Length(script);
	position := 0;
	OopExecute(1, position, 'PUT edge probe');
	outputFile := 'OOPPU00.OUT';
	outputFile[6] := Chr(Ord('0') + (caseId div 10));
	outputFile[7] := Chr(Ord('0') + (caseId mod 10));
	Assign(output, outputFile);
	Rewrite(output);
	WriteLn(output, caseId, ' ', targetX, ' ', targetY, ' ', observedX,
		' ', observedY, ' ', Board.Tiles[observedX][observedY].Element,
		' ', Board.Tiles[observedX][observedY].Color, ' ', Board.Stats[1].Cycle,
		' ', Board.Stats[1].DataPos, ' ', position);
	Close(output);
end.
