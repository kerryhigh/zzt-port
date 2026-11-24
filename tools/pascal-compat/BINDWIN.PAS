program BindWindowAllocationProbe;
uses Crt, Video, GameVars, Oop, Game, TxtWind;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
type
	TString255 = string[255];
	PString255 = ^TString255;
const
	targetSource: string[255] = '@TARGET'#13'#CYCLE 9'#13'#END'#13;
var
	output: text;
	sourceText: string[255];
	outputName: string[20];
	windowLine: TTextWindowLine;
	testWindow: TTextWindowState;
	aliasData: pointer;
	aliasView: PString255;
	dataSize, errorCode, position: integer;
begin
	if ParamCount <> 1 then Halt(2);
	Val(ParamStr(1), dataSize, errorCode);
	if errorCode <> 0 then Halt(3);
	sourceText := '#BIND TARGET'#13'#CYCLE 5'#13'#END'#13;
	while Length(sourceText) < dataSize do sourceText := sourceText + 'X';
	New(IoTmpBuf);
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	WorldCreate;
	BoardCreate;
	Board.StatCount := 3;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Tiles[10][10].Element := E_OBJECT;
	GetMem(Board.Stats[1].Data, Length(sourceText));
	Move(sourceText[1], Board.Stats[1].Data^, Length(sourceText));
	Board.Stats[1].DataLen := Length(sourceText);
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
	position := 0;
	OopExecute(1, position, 'Bind window allocation');
	TextWindowInitState(testWindow);
	windowLine := '01234567890123456789012345678901234567890123456789';
	TextWindowAppend(testWindow, windowLine);
	aliasView := aliasData;
	outputName := 'BW' + ParamStr(1) + '.OUT';
	Assign(output, outputName);
	Rewrite(output);
	WriteLn(output, dataSize, ' ', Ord(testWindow.Lines[1] = aliasData), ' ',
		Length(aliasView^), ' ', Ord(aliasView^[1]), ' ', Ord(aliasView^[2]), ' ',
		Ord(aliasView^[13]), ' ', Board.Stats[1].Cycle);
	Close(output);
end.
