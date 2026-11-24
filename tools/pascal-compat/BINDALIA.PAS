program BindAliasProbe;
uses Crt, Video, GameVars, Oop, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
const
	source: string[255] = '#BIND TARGET'#13'#CYCLE 5'#13'#END'#13;
	targetSource: string[255] = '@TARGET'#13'#CYCLE 9'#13'#END'#13;
var
	output: text;
	position: integer;
	aliasData: pointer;
	ch: char;
	i: integer;
begin
	New(IoTmpBuf);
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	WorldCreate;
	BoardCreate;
	Board.StatCount := 3;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Tiles[10][10].Element := E_OBJECT;
	GetMem(Board.Stats[1].Data, Length(source));
	Move(source[1], Board.Stats[1].Data^, Length(source));
	Board.Stats[1].DataLen := Length(source);
	aliasData := Board.Stats[1].Data;
	Board.Stats[2].X := 11;
	Board.Stats[2].Y := 10;
	Board.Tiles[11][10].Element := E_OBJECT;
	GetMem(Board.Stats[2].Data, Length(targetSource));
	Move(targetSource[1], Board.Stats[2].Data^, Length(targetSource));
	Board.Stats[2].DataLen := Length(targetSource);
	Board.Stats[3].X := 12;
	Board.Stats[3].Y := 10;
	Board.Tiles[12][10].Element := E_OBJECT;
	Board.Stats[3].Data := aliasData;
	Board.Stats[3].DataLen := Board.Stats[1].DataLen;
	Assign(output, 'BINDALIA.OUT');
	Rewrite(output);
	WriteLn(output, 'before');
	Flush(output);
	position := 0;
	OopExecute(1, position, 'Bind alias test');
	Write(output, 'returned ', Board.Stats[1].Cycle, ' ', Board.Stats[1].DataLen);
	WriteLn(output, ' ', Board.Stats[1].DataPos, ' ', Board.Stats[3].DataLen);
	Flush(output);
	position := 13;
	OopExecute(3, position, 'Bind alias follower');
	WriteLn(output, 'follower ', Board.Stats[3].Cycle);
	Flush(output);
	Flush(output);
	position := 13;
	OopExecute(3, position, 'Bind alias follower');
	WriteLn(output, 'follower ', Board.Stats[3].Cycle);
	Flush(output);
	Close(output);
end.
