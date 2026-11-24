program BindReus;
uses Crt, Video, GameVars, Oop, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
const
	source: string[255] = '#BIND TARGET'#13'#CYCLE 5'#13'#END'#13;
	targetSource: string[255] = '@TARGET'#13'#CYCLE 9'#13'#END'#13;
	reusedSource: string[255] = 'XXXXXXXXXXXX'#13'#CYCLE 7'#13'#END'#13;
var
	output: text;
	template: TStat;
	position: integer;
	aliasData: pointer;
	reusedData: pointer;
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
	Board.Stats[3].DataLen := Length(source);
	Assign(output, 'BINDREUS.OUT');
	Rewrite(output);
	WriteLn(output, 'started');
	Flush(output);
	position := 0;
	OopExecute(1, position, 'Bind alias reuse');
	WriteLn(output, 'bound');
	Flush(output);
	FillChar(template, SizeOf(template), 0);
	template.Data := @reusedSource[1];
	template.DataLen := Length(reusedSource);
	AddStat(13, 10, E_OBJECT, $0F, 4, template);
	reusedData := Board.Stats[4].Data;
	if reusedData = aliasData then begin
		WriteLn(output, 'reused 1');
		Flush(output);
		Move(reusedSource[1], reusedData^, Length(reusedSource));
		position := 13;
		OopExecute(3, position, 'Bind alias follower after reuse');
		WriteLn(output, 'follower ', Board.Stats[3].Cycle);
	end else begin
		WriteLn(output, 'reused 0');
		WriteLn(output, 'follower skipped');
		Flush(output);
	end;
	Close(output);
end.
