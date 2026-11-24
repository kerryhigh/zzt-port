program OopChangeProbe;
uses Crt, Video, GameVars, Oop, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	x, y, position: integer;
	script: string[255];
	output: text;

begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	New(IoTmpBuf);
	WorldCreate;
	BoardCreate;
	Board.StatCount := 1;
	Board.Stats[0].X := 11;
	Board.Stats[0].Y := 10;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].Cycle := 3;
	Board.Stats[1].DataPos := 0;
	for y := 0 to BOARD_HEIGHT do
		for x := 0 to BOARD_WIDTH do begin
			Board.Tiles[x][y].Element := E_EMPTY;
			Board.Tiles[x][y].Color := (x * 31 + y * 17) mod 256;
		end;
	Board.Tiles[10][10].Element := E_OBJECT;
	Board.Tiles[10][10].Color := $1F;
	Board.Tiles[11][10].Element := E_PLAYER;
	Board.Tiles[11][10].Color := $0F;
	Board.Tiles[20][20].Element := E_SOLID;
	Board.Tiles[20][20].Color := $3C;
	script := '#CHANGE EMPTY SOLID' + #13 + '#END' + #13;
	GetMem(Board.Stats[1].Data, Length(script));
	Move(script[1], Board.Stats[1].Data^, Length(script));
	Board.Stats[1].DataLen := Length(script);
	position := 0;
	OopExecute(1, position, 'CHANGE map probe');
	if (Board.Info.Message <> '') or (Board.Stats[1].DataPos < 0) then
		Halt(2);
	Assign(output, 'OOPCHG.OUT');
	Rewrite(output);
	for y := 0 to BOARD_HEIGHT do
		for x := 0 to BOARD_WIDTH do
			WriteLn(output, x, ' ', y, ' ', Board.Tiles[x][y].Element, ' ', Board.Tiles[x][y].Color);
	Close(output);
end.
