program OopAnyProbe;
uses Crt, Video, GameVars, Oop, Game, Elements;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
function StripElementName(source: TString50): TString50;
	var
		i: integer;
		ch: char;
		result: TString50;
	begin
		result := '';
		for i := 1 to Length(source) do begin
			ch := UpCase(source[i]);
			if ((ch >= 'A') and (ch <= 'Z')) or ((ch >= '0') and (ch <= '9')) then
				result := result + ch;
		end;
		StripElementName := result;
	end;

procedure FillBoard(element: byte; targetElement: integer; placeTarget: boolean);
	var
		x, y: integer;
	begin
		for x := 1 to BOARD_WIDTH do
			for y := 1 to BOARD_HEIGHT do
				Board.Tiles[x][y].Element := element;
		if placeTarget then
			Board.Tiles[3][3].Element := targetElement;
	end;

var
	elementId, errorCode: integer;
	token, script: string[255];
	position: integer;
	positive, negative: integer;
	fillerElement: byte;
	outputFile: TString50;
	output: text;
begin
	New(IoTmpBuf);
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	Val(ParamStr(1), elementId, errorCode);
	if (errorCode <> 0) or (elementId < 0) or (elementId > MAX_ELEMENT) then
		Halt(1);
	WorldCreate;
	BoardCreate;
	token := StripElementName(ElementDefs[elementId].Name);
	outputFile := 'OOPANY00.OUT';
	outputFile[7] := Chr(Ord('0') + (elementId div 10));
	outputFile[8] := Chr(Ord('0') + (elementId mod 10));
	Assign(output, outputFile);
	Rewrite(output);
	WriteLn(output, 'started');
	Flush(output);
	if Length(token) = 0 then begin
		WriteLn(output, elementId, ' SKIP');
		Close(output);
		Halt;
	end;
	fillerElement := E_EMPTY;
	if elementId = E_EMPTY then
		fillerElement := E_SOLID;
	Board.StatCount := 1;
	Board.Stats[0].X := 5;
	Board.Stats[0].Y := 5;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].Cycle := 0;
	Board.Stats[1].P1 := 0;
	Board.Stats[1].P2 := 0;
	Board.Stats[1].P3 := 0;
	script := '#IF ANY ' + token + ' #CYCLE 7' + #13;
	GetMem(Board.Stats[1].Data, Length(script));
	Move(script[1], Board.Stats[1].Data^, Length(script));
	Board.Stats[1].DataLen := Length(script);
	Board.Stats[1].DataPos := 0;
	FillBoard(fillerElement, elementId, true);
	position := 0;
	OopExecute(1, position, 'ANY element name');
	positive := 0;
	if Board.Stats[1].Cycle = 7 then
		positive := 1;
	Board.Stats[1].Cycle := 0;
	Board.Stats[1].DataPos := 0;
	FillBoard(fillerElement, elementId, false);
	position := 0;
	OopExecute(1, position, 'ANY element name');
	negative := 0;
	if Board.Stats[1].Cycle = 7 then
		negative := 1;
	WriteLn(output, elementId, ' ', token, ' ', positive, ' ', negative);
	Close(output);
end.
