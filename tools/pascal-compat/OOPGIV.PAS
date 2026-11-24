program OopGiveTakeProbe;
uses Crt, Video, GameVars, Oop, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
const
	counterNames: array[0..5] of string[8] =
		('HEALTH', 'AMMO', 'GEMS', 'TORCHES', 'SCORE', 'TIME');
var
	caseId, errorCode, counterId, operation, initialValue, position: integer;
	script: string[255];
	outputFile: TString50;
	output: text;

procedure SetCounter(counterId, value: integer);
	begin
		case counterId of
			0: World.Info.Health := value;
			1: World.Info.Ammo := value;
			2: World.Info.Gems := value;
			3: World.Info.Torches := value;
			4: World.Info.Score := value;
			5: World.Info.BoardTimeSec := value;
		end;
	end;

function GetCounter(counterId: integer): integer;
	begin
		case counterId of
			0: GetCounter := World.Info.Health;
			1: GetCounter := World.Info.Ammo;
			2: GetCounter := World.Info.Gems;
			3: GetCounter := World.Info.Torches;
			4: GetCounter := World.Info.Score;
			5: GetCounter := World.Info.BoardTimeSec;
	end;
	end;

begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	Val(ParamStr(1), caseId, errorCode);
	if (errorCode <> 0) or (caseId < 1) or (caseId > 24) then
		Halt(1);
	counterId := (caseId - 1) div 4;
	operation := (caseId - 1) mod 4;
	case operation of
		0: initialValue := 32767;
		1: initialValue := 32766;
		2: initialValue := 0;
		3: initialValue := 1;
	end;
	New(IoTmpBuf);
	WorldCreate;
	BoardCreate;
	Board.StatCount := 1;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].Cycle := 0;
	Board.Tiles[10][10].Element := E_OBJECT;
	Board.Tiles[10][10].Color := $0F;
	SetCounter(counterId, initialValue);
	if operation < 2 then
		script := '#GIVE ' + counterNames[counterId] + ' 1 #CYCLE 5' + #13
	else
		script := '#TAKE ' + counterNames[counterId] + ' 1 #CYCLE 5' + #13;
	GetMem(Board.Stats[1].Data, Length(script));
	Move(script[1], Board.Stats[1].Data^, Length(script));
	Board.Stats[1].DataLen := Length(script);
	Board.Stats[1].DataPos := 0;
	position := 0;
	OopExecute(1, position, 'Counter case');
	outputFile := 'OOPGIV00.OUT';
	outputFile[7] := Chr(Ord('0') + (caseId div 10));
	outputFile[8] := Chr(Ord('0') + (caseId mod 10));
	Assign(output, outputFile);
	Rewrite(output);
	WriteLn(output, caseId, ' ', GetCounter(counterId), ' ', Board.Stats[1].Cycle);
	Close(output);
end.
