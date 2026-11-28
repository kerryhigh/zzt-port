program OopRandomWalkProbe;
uses Crt, Video, GameVars, Oop, Game;
const caseCount = 6;
var
	position: integer;
	seed: LongInt;
	script: string[32];
	output: text;

{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}

procedure SetCase(source: string; randomSeed: LongInt);
begin
	WorldCreate;
	BoardCreate;
	Board.StatCount := 1;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].Cycle := 1;
	Board.Stats[1].P2 := 0;
	Board.Stats[1].StepX := 0;
	Board.Stats[1].StepY := 0;
	Board.Stats[1].DataPos := 0;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := $70;
	Board.Tiles[10][10].Element := E_OBJECT;
	Board.Tiles[10][10].Color := $0F;
	Board.Tiles[9][10].Element := E_EMPTY;
	Board.Tiles[11][10].Element := E_EMPTY;
	Board.Tiles[10][9].Element := E_EMPTY;
	Board.Tiles[10][11].Element := E_EMPTY;
	World.Info.Health := 10;
	script := source;
	GetMem(Board.Stats[1].Data, Length(script));
	Move(script[1], Board.Stats[1].Data^, Length(script));
	Board.Stats[1].DataLen := Length(script);
	RandSeed := randomSeed;
	position := 0;
end;

procedure Capture(caseId: integer);
begin
	WriteLn(output, caseId, ' ', position, ' ', Board.Stats[1].X, ' ', Board.Stats[1].Y,
		' ', Board.Stats[1].StepX, ' ', Board.Stats[1].StepY, ' ', Board.Stats[1].DataPos,
		' ', Board.Tiles[10][10].Element, ' ', Board.Tiles[Board.Stats[1].X][Board.Stats[1].Y].Element,
		' ', (RandSeed and $FFFF), ' ', ((RandSeed shr 16) and $FFFF));
end;

begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	New(IoTmpBuf);
	Assign(output, 'OOPRND.OUT');
	Rewrite(output);

	seed := 0;
	SetCase('/RNDNS' + #13, seed);
	OopExecute(1, position, 'Random direction probe');
	Capture(1);

	seed := 2;
	SetCase('/RNDNS' + #13, seed);
	OopExecute(1, position, 'Random direction probe');
	Capture(2);

	seed := 0;
	SetCase('/RNDNE' + #13, seed);
	OopExecute(1, position, 'Random direction probe');
	Capture(3);

	seed := 2;
	SetCase('/RNDNE' + #13, seed);
	OopExecute(1, position, 'Random direction probe');
	Capture(4);

	seed := 0;
	SetCase('/RNDP NORTH' + #13, seed);
	OopExecute(1, position, 'Random direction probe');
	Capture(5);

	seed := 2;
	SetCase('/RNDP NORTH' + #13, seed);
	OopExecute(1, position, 'Random direction probe');
	Capture(6);

	Close(output);
end.
