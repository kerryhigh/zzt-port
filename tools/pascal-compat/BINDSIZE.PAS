program BindSize;
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
	replacementSmall: string[255] = 'XCYCLE 7'#13'#END'#13;
	replacementMedium: string[255] = 'XXXXXXXXXXXX'#13'#CYCLE 7'#13'#END'#13;
	replacementLarge: string[255] = 'XXXXXXXXXXXXXXXXXXXXX'#13'#CYCLE 7'#13'#END'#13;
var
	output: text;
	template: TStat;
	position: integer;
	scenario: string[8];
	sourceDataSmall, sourceDataLarge, replacementData: pointer;
	replacementExtra: string[255];
begin
	New(IoTmpBuf);
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	FillChar(replacementExtra, SizeOf(replacementExtra), Ord('X'));
	replacementExtra[0] := Chr(70);
	WorldCreate;
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
	OopExecute(1, position, 'Bind smaller source');
	position := 0;
	OopExecute(2, position, 'Bind larger source');
	scenario := ParamStr(1);
	if scenario = 'SML' then begin
		template.Data := @replacementSmall[1];
		template.DataLen := Length(replacementSmall);
	end else if scenario = 'MED' then begin
		template.Data := @replacementMedium[1];
		template.DataLen := Length(replacementMedium);
	end else if scenario = 'LRG' then begin
		template.Data := @replacementLarge[1];
		template.DataLen := Length(replacementLarge);
	end else if scenario = 'XL' then begin
		template.Data := @replacementExtra[1];
		template.DataLen := Length(replacementExtra);
	end else if scenario = 'F41' then begin
		replacementExtra[0] := Chr(41);
		template.Data := @replacementExtra[1];
		template.DataLen := 41;
	end else if scenario = 'F45' then begin
		replacementExtra[0] := Chr(45);
		template.Data := @replacementExtra[1];
		template.DataLen := 45;
	end else if scenario = 'F50' then begin
		replacementExtra[0] := Chr(50);
		template.Data := @replacementExtra[1];
		template.DataLen := 50;
	end else if scenario = 'F55' then begin
		replacementExtra[0] := Chr(55);
		template.Data := @replacementExtra[1];
		template.DataLen := 55;
	end else if scenario = 'F60' then begin
		replacementExtra[0] := Chr(60);
		template.Data := @replacementExtra[1];
		template.DataLen := 60;
	end else if scenario = 'F63' then begin
		replacementExtra[0] := Chr(63);
		template.Data := @replacementExtra[1];
		template.DataLen := 63;
	end else if scenario = 'F64' then begin
		replacementExtra[0] := Chr(64);
		template.Data := @replacementExtra[1];
		template.DataLen := 64;
	end else begin
		Halt(1);
	end;
	replacementData := template.Data;
	AddStat(15, 10, E_OBJECT, $0F, 4, template);
	Assign(output, 'SIZ' + scenario + '.OUT');
	Rewrite(output);
	WriteLn(output, scenario, ' ', Board.Stats[6].DataLen, ' ',
		Ord(Board.Stats[6].Data = sourceDataSmall), ' ',
		Ord(Board.Stats[6].Data = sourceDataLarge), ' ',
		Ord(Board.Stats[6].Data = replacementData), ' ',
		Ord(PByte(sourceDataSmall)^), ' ', Ord(PByte(sourceDataLarge)^));
	Close(output);
end.
