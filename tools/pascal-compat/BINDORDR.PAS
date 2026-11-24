program BindOrder;
uses Crt, Video, GameVars, Oop, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
const
	sourceOne: string[255] = '#BIND TARGET'#13'#CYCLE 5'#13'#END'#13;
	sourceTwo: string[255] = '#BIND TARGET'#13'#CYCLE 5'#13'#END'#13;
	targetSource: string[255] = '@TARGET'#13'#CYCLE 9'#13'#END'#13;
	replacementOne: string[255] = 'XXXXXXXXXXXX'#13'#CYCLE 7'#13'#END'#13;
	replacementTwo: string[255] = 'XXXXXXXXXXXX'#13'#CYCLE 8'#13'#END'#13;
var
	output: text;
	template: TStat;
	position: integer;
	sourceDataOne, sourceDataTwo: pointer;
	firstData, secondData: pointer;
begin
	New(IoTmpBuf);
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	WorldCreate;
	BoardCreate;
	Board.StatCount := 5;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Tiles[10][10].Element := E_OBJECT;
	GetMem(Board.Stats[1].Data, Length(sourceOne));
	Move(sourceOne[1], Board.Stats[1].Data^, Length(sourceOne));
	Board.Stats[1].DataLen := Length(sourceOne);
	sourceDataOne := Board.Stats[1].Data;
	Board.Stats[2].X := 11;
	Board.Stats[2].Y := 10;
	Board.Tiles[11][10].Element := E_OBJECT;
	GetMem(Board.Stats[2].Data, Length(sourceTwo));
	Move(sourceTwo[1], Board.Stats[2].Data^, Length(sourceTwo));
	Board.Stats[2].DataLen := Length(sourceTwo);
	sourceDataTwo := Board.Stats[2].Data;
	Board.Stats[3].X := 12;
	Board.Stats[3].Y := 10;
	Board.Tiles[12][10].Element := E_OBJECT;
	GetMem(Board.Stats[3].Data, Length(targetSource));
	Move(targetSource[1], Board.Stats[3].Data^, Length(targetSource));
	Board.Stats[3].DataLen := Length(targetSource);
	Board.Stats[4].X := 13;
	Board.Stats[4].Y := 10;
	Board.Tiles[13][10].Element := E_OBJECT;
	Board.Stats[4].Data := sourceDataOne;
	Board.Stats[4].DataLen := Length(sourceOne);
	Board.Stats[5].X := 14;
	Board.Stats[5].Y := 10;
	Board.Tiles[14][10].Element := E_OBJECT;
	Board.Stats[5].Data := sourceDataTwo;
	Board.Stats[5].DataLen := Length(sourceTwo);
	Assign(output, 'BINDORDR.OUT');
	Rewrite(output);
	WriteLn(output, 'started');
	Flush(output);
	position := 0;
	OopExecute(1, position, 'Bind first source');
	position := 0;
	OopExecute(2, position, 'Bind second source');
	WriteLn(output, 'bound');
	Flush(output);
	FillChar(template, SizeOf(template), 0);
	template.Data := @replacementOne[1];
	template.DataLen := Length(replacementOne);
	AddStat(15, 10, E_OBJECT, $0F, 4, template);
	firstData := Board.Stats[6].Data;
	template.Data := @replacementTwo[1];
	AddStat(16, 10, E_OBJECT, $0F, 4, template);
	secondData := Board.Stats[7].Data;
	WriteLn(output, 'reuse ', Ord(firstData = sourceDataOne), ' ', Ord(firstData = sourceDataTwo), ' ',
		Ord(secondData = sourceDataOne), ' ', Ord(secondData = sourceDataTwo));
	position := 13;
	OopExecute(4, position, 'First alias after two reuse allocations');
	position := 13;
	OopExecute(5, position, 'Second alias after two reuse allocations');
	WriteLn(output, 'followers ', Board.Stats[4].Cycle, ' ', Board.Stats[5].Cycle);
	Close(output);
end.
