program BindProbe;
uses Crt, Video, GameVars, Oop, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
const
	script: string[255] = '#BIND SELF'#13'#CYCLE 7'#13'#END'#13;
	bindScript: string[255] = '#BIND TARGET'#13'#CYCLE 5'#13'#END'#13;
	targetScript: string[255] = '@TARGET'#13'#CYCLE 9'#13'#END'#13;
var
	output: text;
	position: integer;
begin
	New(IoTmpBuf);
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	WorldCreate;
	Board.StatCount := 1;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Tiles[10][10].Element := E_OBJECT;
	Board.Tiles[10][10].Color := $0F;
	GetMem(Board.Stats[1].Data, Length(script));
	Move(script[1], Board.Stats[1].Data^, Length(script));
	Board.Stats[1].DataLen := Length(script);
	position := 0;
	OopExecute(1, position, 'Test');
	Assign(output, 'BINDPROB.OUT');
	Rewrite(output);
	WriteLn(output, Board.Stats[1].Cycle, ' ', Board.Stats[1].DataLen, ' ', Board.Stats[1].DataPos);
	BoardCreate;
	Board.StatCount := 3;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Tiles[10][10].Element := E_OBJECT;
	GetMem(Board.Stats[1].Data, Length(bindScript));
	Move(bindScript[1], Board.Stats[1].Data^, Length(bindScript));
	Board.Stats[1].DataLen := Length(bindScript);
	Board.Stats[2].X := 11;
	Board.Stats[2].Y := 10;
	Board.Tiles[11][10].Element := E_OBJECT;
	GetMem(Board.Stats[2].Data, Length(targetScript));
	Move(targetScript[1], Board.Stats[2].Data^, Length(targetScript));
	Board.Stats[2].DataLen := Length(targetScript);
	Board.StatCount := 2;
	position := 0;
	OopExecute(1, position, 'Test');
	WriteLn(output, Board.Stats[1].Cycle, ' ', Board.Stats[1].DataLen, ' ', Board.Stats[1].DataPos);
	Close(output);
end.
