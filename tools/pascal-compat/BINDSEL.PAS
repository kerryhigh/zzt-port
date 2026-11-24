program BindSelectorProbe;
uses Crt, Video, GameVars, Oop, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
const
	bindOthers: string[255] = '#BIND OTHERS'#13'#CYCLE 5'#13'#END'#13;
	othersTarget: string[255] = '@OTHER'#13'#CYCLE 9'#13'#END'#13;
	othersFromSecond: string[255] = '#BIND OTHERS'#13'#CYCLE 5'#13'#END'#13;
	othersSecondTarget: string[255] = '@FIRST'#13'#CYCLE 13'#13'#END'#13;
	bindAll: string[255] = '#BIND ALL'#13'#CYCLE 5'#13'#END'#13;
	allTarget: string[255] = '@FIRST'#13'#CYCLE 7'#13'#END'#13;
	bindNamed: string[255] = '#BIND TARGET'#13'#CYCLE 5'#13'#END'#13;
	namedTargetFirst: string[255] = '@target'#13'#CYCLE 9'#13'#END'#13;
	namedTargetSecond: string[255] = '@TARGET'#13'#CYCLE 11'#13'#END'#13;
	bindMissing: string[255] = '#BIND MISSING'#13'#CYCLE 5'#13'#END'#13;
var
	output: text;
	outputName: string[12];
	caseId, errorCode, i: integer;
	position: integer;

procedure InstallScript(statId: integer; script: string);
begin
	Board.Stats[statId].X := 10 + statId;
	Board.Stats[statId].Y := 10;
	Board.Stats[statId].Cycle := 0;
	Board.Stats[statId].DataPos := 0;
	Board.Tiles[10 + statId][10].Element := E_OBJECT;
	if Length(script) <> 0 then begin
		GetMem(Board.Stats[statId].Data, Length(script));
		Move(script[1], Board.Stats[statId].Data^, Length(script));
		Board.Stats[statId].DataLen := Length(script);
	end;
end;

begin
	New(IoTmpBuf);
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	Val(ParamStr(1), caseId, errorCode);
	if (errorCode <> 0) or (caseId < 1) or (caseId > 5) then
		Halt(1);
	WorldCreate;
	BoardCreate;
	for i := 1 to 3 do begin
		Board.Stats[i].Cycle := 0;
		Board.Stats[i].DataLen := 0;
		Board.Stats[i].DataPos := 0;
	end;
	case caseId of
		1: begin
			Board.StatCount := 2;
			InstallScript(1, bindOthers);
			InstallScript(2, othersTarget);
			outputName := 'BINDSE1.OUT';
		end;
		2: begin
			Board.StatCount := 2;
			InstallScript(1, allTarget);
			InstallScript(2, bindAll);
			outputName := 'BINDSE2.OUT';
		end;
		3: begin
			Board.StatCount := 3;
			InstallScript(1, bindNamed);
			InstallScript(2, namedTargetFirst);
			InstallScript(3, namedTargetSecond);
			outputName := 'BINDSE3.OUT';
		end;
		4: begin
			Board.StatCount := 1;
			InstallScript(1, bindMissing);
			outputName := 'BINDSE4.OUT';
		end;
		5: begin
			Board.StatCount := 2;
			InstallScript(1, othersSecondTarget);
			InstallScript(2, othersFromSecond);
			outputName := 'BINDSE5.OUT';
		end;
	end;
	position := 0;
	if (caseId = 2) or (caseId = 5) then
		OopExecute(2, position, 'Bind selector')
	else
		OopExecute(1, position, 'Bind selector');
	Assign(output, outputName);
	Rewrite(output);
	if (caseId = 2) or (caseId = 5) then
		WriteLn(output, caseId, ' ', Board.Stats[1].Cycle, ' ', Board.Stats[2].Cycle,
			' ', Board.Stats[3].Cycle, ' ', Board.Stats[2].DataLen)
	else
		WriteLn(output, caseId, ' ', Board.Stats[1].Cycle, ' ', Board.Stats[2].Cycle,
			' ', Board.Stats[3].Cycle, ' ', Board.Stats[1].DataLen);
	Close(output);
end.
