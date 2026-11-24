program OopIfProbe;
uses Crt, Video, GameVars, Oop, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
procedure RunCase(caseId: integer; condition: TString50; energized: integer;
	statX, statY, playerX, playerY: integer;
	obstacleX, obstacleY, obstacleElement, obstacleColor: integer;
	flag: TString50; var output: text);
	var
		script: string[255];
		position: integer;
		actual: integer;
	begin
		WorldCreate;
		BoardCreate;
		Board.Tiles[BOARD_WIDTH div 2][BOARD_HEIGHT div 2].Element := E_EMPTY;
		Board.StatCount := 1;
		Board.Stats[0].X := playerX;
		Board.Stats[0].Y := playerY;
		Board.Stats[1].X := statX;
		Board.Stats[1].Y := statY;
		Board.Stats[1].Cycle := 0;
		Board.Stats[1].P1 := 0;
		Board.Stats[1].P2 := 0;
		Board.Stats[1].P3 := 0;
		Board.Tiles[statX][statY].Element := E_OBJECT;
		Board.Tiles[statX][statY].Color := $0F;
		World.Info.EnergizerTicks := energized;
		if (obstacleX <> 0) or (obstacleY <> 0) then begin
			Board.Tiles[obstacleX][obstacleY].Element := obstacleElement;
			Board.Tiles[obstacleX][obstacleY].Color := obstacleColor;
		end;
		if Length(flag) <> 0 then
			WorldSetFlag(flag);
		script := '#IF ' + condition + ' #CYCLE 7' + #13;
		GetMem(Board.Stats[1].Data, Length(script));
		Move(script[1], Board.Stats[1].Data^, Length(script));
		Board.Stats[1].DataLen := Length(script);
		Board.Stats[1].DataPos := 0;
		position := 0;
		OopExecute(1, position, 'IF condition test');
		actual := 0;
		if Board.Stats[1].Cycle = 7 then
			actual := 1;
		WriteLn(output, caseId, ' ', actual);
	end;
var
	caseId, errorCode: integer;
	outputFile: TString50;
	output: text;
begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
		New(IoTmpBuf);
	Val(ParamStr(1), caseId, errorCode);
	if (errorCode <> 0) or (caseId < 1) or (caseId > 55) then
		Halt(1);
	outputFile := 'OOPIF00.OUT';
	outputFile[6] := Chr(Ord('0') + (caseId div 10));
	outputFile[7] := Chr(Ord('0') + (caseId mod 10));
	Assign(output, outputFile);
	Rewrite(output);
	WriteLn(output, 'started');
	Flush(output);
	case caseId of
		1: RunCase(1, 'ENERGIZED', 1, 10, 10, 5, 5, 0, 0, E_EMPTY, 0, '', output);
		2: RunCase(2, 'ENERGIZED', 0, 10, 10, 5, 5, 0, 0, E_EMPTY, 0, '', output);
		3: RunCase(3, 'NOT ENERGIZED', 0, 10, 10, 5, 5, 0, 0, E_EMPTY, 0, '', output);
		4: RunCase(4, 'NOT ENERGIZED', 1, 10, 10, 5, 5, 0, 0, E_EMPTY, 0, '', output);
		5: RunCase(5, 'ALLIGNED', 0, 10, 10, 10, 5, 0, 0, E_EMPTY, 0, '', output);
		6: RunCase(6, 'ALLIGNED', 0, 10, 10, 5, 5, 0, 0, E_EMPTY, 0, '', output);
		7: RunCase(7, 'CONTACT', 0, 10, 10, 11, 10, 0, 0, E_EMPTY, 0, '', output);
		8: RunCase(8, 'CONTACT', 0, 10, 10, 12, 10, 0, 0, E_EMPTY, 0, '', output);
		9: RunCase(9, 'BLOCKED NORTH', 0, 10, 10, 5, 5, 10, 9, E_SOLID, 0, '', output);
		10: RunCase(10, 'BLOCKED NORTH', 0, 10, 10, 5, 5, 0, 0, E_EMPTY, 0, '', output);
		11: RunCase(11, 'ANY SOLID', 0, 10, 10, 5, 5, 3, 3, E_SOLID, 0, '', output);
		12: RunCase(12, 'ANY SOLID', 0, 10, 10, 5, 5, 0, 0, E_EMPTY, 0, '', output);
		13: RunCase(13, 'ANY BLUE DOOR', 0, 10, 10, 5, 5, 3, 3, E_DOOR, $1F, '', output);
		14: RunCase(14, 'ANY BLUE DOOR', 0, 10, 10, 5, 5, 3, 3, E_DOOR, $2F, '', output);
		15: RunCase(15, 'ANY GREEN DOOR', 0, 10, 10, 5, 5, 3, 3, E_DOOR, $2F, '', output);
		16: RunCase(16, 'ANY GREEN DOOR', 0, 10, 10, 5, 5, 3, 3, E_DOOR, $1F, '', output);
		17: RunCase(17, 'ANY CYAN DOOR', 0, 10, 10, 5, 5, 3, 3, E_DOOR, $3F, '', output);
		18: RunCase(18, 'ANY CYAN DOOR', 0, 10, 10, 5, 5, 3, 3, E_DOOR, $1F, '', output);
		19: RunCase(19, 'ANY RED DOOR', 0, 10, 10, 5, 5, 3, 3, E_DOOR, $4F, '', output);
		20: RunCase(20, 'ANY RED DOOR', 0, 10, 10, 5, 5, 3, 3, E_DOOR, $1F, '', output);
		21: RunCase(21, 'ANY PURPLE DOOR', 0, 10, 10, 5, 5, 3, 3, E_DOOR, $5F, '', output);
		22: RunCase(22, 'ANY PURPLE DOOR', 0, 10, 10, 5, 5, 3, 3, E_DOOR, $1F, '', output);
		23: RunCase(23, 'ANY YELLOW DOOR', 0, 10, 10, 5, 5, 3, 3, E_DOOR, $6F, '', output);
		24: RunCase(24, 'ANY YELLOW DOOR', 0, 10, 10, 5, 5, 3, 3, E_DOOR, $1F, '', output);
		25: RunCase(25, 'ANY WHITE DOOR', 0, 10, 10, 5, 5, 3, 3, E_DOOR, $7F, '', output);
		26: RunCase(26, 'ANY WHITE DOOR', 0, 10, 10, 5, 5, 3, 3, E_DOOR, $1F, '', output);
		27: RunCase(27, 'HELLO', 0, 10, 10, 5, 5, 0, 0, E_EMPTY, 0, 'HELLO', output);
		28: RunCase(28, 'HELLO', 0, 10, 10, 5, 5, 0, 0, E_EMPTY, 0, '', output);
		29: RunCase(29, 'ALLIGNED', 0, 10, 10, 5, 10, 0, 0, E_EMPTY, 0, '', output);
		30: RunCase(30, 'CONTACT', 0, 10, 10, 10, 9, 0, 0, E_EMPTY, 0, '', output);
		31: RunCase(31, 'CONTACT', 0, 10, 10, 11, 11, 0, 0, E_EMPTY, 0, '', output);
		32: RunCase(32, 'BLOCKED SOUTH', 0, 10, 10, 5, 5, 10, 11, E_SOLID, 0, '', output);
		33: RunCase(33, 'BLOCKED WEST', 0, 10, 10, 5, 5, 9, 10, E_EMPTY, 0, '', output);
		34: RunCase(34, 'ANY WATER', 0, 10, 10, 5, 5, 3, 3, E_WATER, 0, '', output);
		35: RunCase(35, 'ANY WATER', 0, 10, 10, 5, 5, 0, 0, E_EMPTY, 0, '', output);
		36: RunCase(36, 'ANY BLUE KEY', 0, 10, 10, 5, 5, 3, 3, E_KEY, $19, '', output);
		37: RunCase(37, 'ANY BLUE KEY', 0, 10, 10, 5, 5, 3, 3, E_KEY, $1C, '', output);
		38: RunCase(38, 'BLOCKED EAST', 0, 10, 10, 5, 5, 11, 10, E_SOLID, 0, '', output);
		39: RunCase(39, 'BLOCKED EAST', 0, 10, 10, 5, 5, 0, 0, E_EMPTY, 0, '', output);
		40: RunCase(40, 'BLOCKED WEST', 0, 10, 10, 5, 5, 9, 10, E_SOLID, 0, '', output);
		41: RunCase(41, 'BLOCKED SOUTH', 0, 10, 10, 5, 5, 0, 0, E_EMPTY, 0, '', output);
		42: RunCase(42, 'BLOCKED WEST', 0, 1, 10, 5, 5, 0, 10, E_BOARD_EDGE, 0, '', output);
		43: RunCase(43, 'BLOCKED EAST', 0, 60, 10, 5, 5, 61, 10, E_BOARD_EDGE, 0, '', output);
		44: RunCase(44, 'BLOCKED NORTH', 0, 10, 1, 5, 5, 10, 0, E_BOARD_EDGE, 0, '', output);
		45: RunCase(45, 'BLOCKED SOUTH', 0, 10, 25, 5, 5, 10, 26, E_BOARD_EDGE, 0, '', output);
		46: RunCase(46, 'ANY SOLID', 0, 10, 10, 5, 5, 60, 25, E_SOLID, 0, '', output);
		47: RunCase(47, 'ANY SOLID', 0, 10, 10, 5, 5, 0, 10, E_SOLID, 0, '', output);
		48: RunCase(48, 'NOT NOT ENERGIZED', 0, 10, 10, 5, 5, 0, 0, E_EMPTY, 0, '', output);
		49: RunCase(49, 'NOT NOT ENERGIZED', 1, 10, 10, 5, 5, 0, 0, E_EMPTY, 0, '', output);
		50: RunCase(50, 'NOT NOT NOT ENERGIZED', 0, 10, 10, 5, 5, 0, 0, E_EMPTY, 0, '', output);
		51: RunCase(51, 'NOT NOT NOT ENERGIZED', 1, 10, 10, 5, 5, 0, 0, E_EMPTY, 0, '', output);
		52: RunCase(52, 'NOT NOT BLOCKED NORTH', 0, 10, 10, 5, 5, 10, 9, E_SOLID, 0, '', output);
		53: RunCase(53, 'NOT NOT BLOCKED NORTH', 0, 10, 10, 5, 5, 0, 0, E_EMPTY, 0, '', output);
		54: RunCase(54, 'NOT NOT HELLO', 0, 10, 10, 5, 5, 0, 0, E_EMPTY, 0, 'HELLO', output);
		55: RunCase(55, 'NOT NOT HELLO', 0, 10, 10, 5, 5, 0, 0, E_EMPTY, 0, '', output);
	end;
	Close(output);
	Dispose(IoTmpBuf);
end.
