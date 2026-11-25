program OopFlowProbe;
uses Crt, Video, GameVars, Oop, Game;
const caseCount = 11;
var
	position, caseId: integer;
	script: string[255];
	output: text;

{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}

procedure SetCase(source: string; eastElement: byte);
begin
	WorldCreate;
	BoardCreate;
	Board.StatCount := 1;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].Cycle := 3;
	Board.Stats[1].P2 := 0;
	Board.Stats[1].StepX := 0;
	Board.Stats[1].StepY := 0;
	Board.Stats[1].DataPos := 0;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := $70;
	Board.Tiles[10][10].Element := E_OBJECT;
	Board.Tiles[10][10].Color := $0F;
	Board.Tiles[11][10].Element := eastElement;
	Board.Tiles[11][10].Color := $2A;
	World.Info.Health := 10;
	script := source;
	if Length(script) <> 0 then begin
		GetMem(Board.Stats[1].Data, Length(script));
		Move(script[1], Board.Stats[1].Data^, Length(script));
	end;
	Board.Stats[1].DataLen := Length(script);
	position := 0;
end;

procedure Capture(caseId: integer);
begin
	WriteLn(output, caseId, ' ', position, ' ', Board.Stats[1].Cycle,
		' ', World.Info.Health, ' ', Board.Stats[1].P2,
		' ', Board.Stats[1].X, ' ', Board.Stats[1].Y,
		' ', Board.Stats[1].StepX, ' ', Board.Stats[1].StepY,
		' ', Board.Tiles[10][10].Element, ' ', Board.Tiles[11][10].Element);
end;

begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	New(IoTmpBuf);
	Assign(output, 'OOPFLOW.OUT');
	Rewrite(output);

	SetCase('#IDLE' + #13 + '#CYCLE 7' + #13, E_EMPTY);
	OopExecute(1, position, 'Flow probe');
	Capture(1);
	OopExecute(1, position, 'Flow probe');
	Capture(2);

	SetCase('#END' + #13 + '#CYCLE 9' + #13, E_EMPTY);
	OopExecute(1, position, 'Flow probe');
	Capture(3);

	SetCase('#ENDGAME' + #13 + '#CYCLE 7' + #13, E_EMPTY);
	OopExecute(1, position, 'Flow probe');
	Capture(4);

	SetCase('#LOCK' + #13 + '#IDLE' + #13 + '#UNLOCK' + #13 + '#END' + #13, E_EMPTY);
	OopExecute(1, position, 'Flow probe');
	Capture(5);
	OopExecute(1, position, 'Flow probe');
	Capture(6);

	SetCase('#RESTART' + #13 + '#CYCLE 9' + #13, E_EMPTY);
	OopExecute(1, position, 'Flow probe');
	Capture(7);

	SetCase('#GO EAST #CYCLE 5' + #13, E_BOARD_EDGE);
	OopExecute(1, position, 'Flow probe');
	Capture(8);

	SetCase('#GO EAST #CYCLE 5' + #13, E_EMPTY);
	OopExecute(1, position, 'Flow probe');
	Capture(9);

	SetCase('#TRY EAST #CYCLE 5' + #13, E_BOARD_EDGE);
	OopExecute(1, position, 'Flow probe');
	Capture(10);

	SetCase('#TRY EAST #CYCLE 5' + #13, E_EMPTY);
	OopExecute(1, position, 'Flow probe');
	Capture(11);

	Close(output);
end.
