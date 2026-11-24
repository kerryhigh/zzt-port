program ConveyorTickProbe;
uses Crt, Video, GameVars, Elements, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
const
	RingX: array[0..7] of integer = (-1, 0, 1, 1, 1, 0, -1, -1);
	RingY: array[0..7] of integer = (1, 1, 1, 0, -1, -1, -1, 0);
var
	output: text;
	i: integer;

procedure SetupConveyorTick(conveyorElement, boulderIndex: integer; boulderColor: byte);
var
	ringIndex: integer;
begin
	BoardCreate;
	Board.StatCount := 1;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := $70;
	Board.Tiles[10][10].Element := conveyorElement;
	Board.Tiles[10][10].Color := $07;
	for ringIndex := 0 to 7 do begin
		Board.Tiles[10 + RingX[ringIndex]][10 + RingY[ringIndex]].Element := E_EMPTY;
		Board.Tiles[10 + RingX[ringIndex]][10 + RingY[ringIndex]].Color := $70;
	end;
	Board.Tiles[10 + RingX[boulderIndex]][10 + RingY[boulderIndex]].Element := E_BOULDER;
	Board.Tiles[10 + RingX[boulderIndex]][10 + RingY[boulderIndex]].Color := boulderColor;
end;

procedure WriteConveyorTick(caseId: integer);
var
	ringIndex, x, y: integer;
begin
	Write(output, 'conveyor-tick ', caseId, ' ', Board.StatCount, ' ',
		Board.Stats[1].X, ' ', Board.Stats[1].Y, ' ',
		Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color);
	for ringIndex := 0 to 7 do begin
		x := 10 + RingX[ringIndex];
		y := 10 + RingY[ringIndex];
		Write(output, ' ', Board.Tiles[x][y].Element, ' ', Board.Tiles[x][y].Color);
	end;
	WriteLn(output);
end;

begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	TickTimeDuration := 4;
	New(IoTmpBuf);
	WorldCreate;
	Assign(output, 'CONVPRB.OUT');
	Rewrite(output);

	SetupConveyorTick(E_CONVEYOR_CW, 0, $1E);
	ElementDefs[E_CONVEYOR_CW].TickProc(1);
	WriteConveyorTick(1);

	SetupConveyorTick(E_CONVEYOR_CCW, 7, $2E);
	ElementDefs[E_CONVEYOR_CCW].TickProc(1);
	WriteConveyorTick(2);

	Close(output);
	Dispose(IoTmpBuf);
end.
