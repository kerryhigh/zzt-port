program BindTailProbe;
uses Crt, Video, GameVars, Oop, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
type
	PByte = ^Byte;
const
	sourceSmall: string[255] = '#BIND TARGET'#13'#CYCLE 5'#13'#END'#13;
	sourceLarge: string[255] = '#BIND TARGET'#13'#CYCLE 5'#13'#END'#13'123456789';
	targetSource: string[255] = '@TARGET'#13'#CYCLE 9'#13'#END'#13;
var
	output: text;
	template: TStat;
	position, requestSize, tailSize, errorCode: integer;
	pointerDelta: longint;
	scenario: string[3];
	sourceDataSmall, sourceDataLarge, firstData, secondData: pointer;
	replacement, tailSource: string[255];
	i: integer;
function LinearAddress(value: PByte): longint;
begin
	LinearAddress := (LongInt(Seg(value^)) * 16) + LongInt(Ofs(value^));
end;
begin
	New(IoTmpBuf);
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	WorldCreate;
	scenario := ParamStr(1);
	if (Length(scenario) <> 3) or (scenario[1] <> 'T') then Halt(1);
	Val(Copy(scenario, 2, 2), requestSize, errorCode);
	if (errorCode <> 0) or (requestSize < 26) or (requestSize > 64) then Halt(1);
	BoardCreate;
	Board.StatCount := 5;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Tiles[10][10].Element := E_OBJECT;
	GetMem(Board.Stats[1].Data, Length(sourceSmall));
	Move(sourceSmall[1], Board.Stats[1].Data^, Length(sourceSmall));
	Board.Stats[1].DataLen := Length(sourceSmall);
	sourceDataSmall := Board.Stats[1].Data;
	Board.Stats[2].X := 11;
	Board.Stats[2].Y := 10;
	Board.Tiles[11][10].Element := E_OBJECT;
	GetMem(Board.Stats[2].Data, Length(sourceLarge));
	Move(sourceLarge[1], Board.Stats[2].Data^, Length(sourceLarge));
	Board.Stats[2].DataLen := Length(sourceLarge);
	sourceDataLarge := Board.Stats[2].Data;
	Board.Stats[3].X := 12;
	Board.Stats[3].Y := 10;
	Board.Tiles[12][10].Element := E_OBJECT;
	GetMem(Board.Stats[3].Data, Length(targetSource));
	Move(targetSource[1], Board.Stats[3].Data^, Length(targetSource));
	Board.Stats[3].DataLen := Length(targetSource);
	Board.Stats[4].X := 13;
	Board.Stats[4].Y := 10;
	Board.Tiles[13][10].Element := E_OBJECT;
	Board.Stats[4].Data := sourceDataSmall;
	Board.Stats[4].DataLen := Length(sourceSmall);
	Board.Stats[5].X := 14;
	Board.Stats[5].Y := 10;
	Board.Tiles[14][10].Element := E_OBJECT;
	Board.Stats[5].Data := sourceDataLarge;
	Board.Stats[5].DataLen := Length(sourceLarge);
	position := 0;
	OopExecute(1, position, 'Bind small block');
	position := 0;
	OopExecute(2, position, 'Bind large block');
	FillChar(template, SizeOf(template), 0);
	FillChar(replacement, SizeOf(replacement), Ord('X'));
	replacement[0] := Chr(requestSize);
	template.Data := @replacement[1];
	template.DataLen := requestSize;
	AddStat(15, 10, E_OBJECT, $0F, 4, template);
	firstData := Board.Stats[6].Data;
	tailSize := 63 - requestSize;
	FillChar(tailSource, SizeOf(tailSource), Ord('Y'));
	if tailSize <= 0 then tailSize := 1;
	tailSource[0] := Chr(tailSize);
	template.Data := @tailSource[1];
	template.DataLen := tailSize;
	AddStat(16, 10, E_OBJECT, $0F, 4, template);
	secondData := Board.Stats[7].Data;
	pointerDelta := LinearAddress(PByte(secondData)) - LinearAddress(PByte(firstData));
	Assign(output, scenario + '.OUT');
	Rewrite(output);
	WriteLn(output, scenario, ' ', Ord(firstData = sourceDataSmall), ' ',
		pointerDelta, ' ', Ord(secondData = sourceDataLarge));
	Close(output);
end.
