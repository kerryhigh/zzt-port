program ScrollTouchTitleProbe;
{$M 32768,0,655360}
uses Crt, Video, GameVars, Elements, Game, Sounds, TxtWind;
var
	output: text;
	capturedTitle: string[50];
	capturedLines: array[1 .. 2] of string[50];
	titleWriteCount: integer;
	lineCaptureCount: integer;
	deltaX, deltaY: integer;
	script: string[255];
{$F+}
procedure CaptureWrite(x, y, color: byte; text: TVideoLine);
begin
	if (y = TextWindowY + 1) and (color = $1E) and (Length(text) <> 0) then begin
		capturedTitle := text;
		titleWriteCount := titleWriteCount + 1;
	end;
	if (x = TextWindowX + 4) and (y > TextWindowY + 3)
		and (color = $1E) and (Length(text) <> 0) then begin
		lineCaptureCount := lineCaptureCount + 1;
		if lineCaptureCount <= 2 then
			capturedLines[lineCaptureCount] := text;
	end;
end;
{$F-}
procedure RunCase(labelText: string; scriptText: string);
var
	charIndex: integer;
	lineIndex: integer;
begin
	WorldCreate;
	VideoWriteText := CaptureWrite;
	TextWindowInit(14, 4, 50, 16);
	Board.StatCount := 1;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].Cycle := 0;
	Board.Stats[1].DataPos := 0;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := $70;
	script := scriptText;
	GetMem(Board.Stats[1].Data, Length(script));
	Move(script[1], Board.Stats[1].Data^, Length(script));
	Board.Stats[1].DataLen := Length(script);
	Board.Tiles[10][10].Element := E_SCROLL;
	Board.Tiles[10][10].Color := $1F;
	CurrentStatTicked := 0;
	TextWindowProbeAutoEscape := true;
	capturedTitle := '';
	titleWriteCount := 0;
	lineCaptureCount := 0;
	capturedLines[1] := '';
	capturedLines[2] := '';
	deltaX := 2;
	deltaY := -1;
	ElementDefs[E_SCROLL].TouchProc(10, 10, 0, deltaX, deltaY);
	if titleWriteCount = 0 then Halt(2);
	if lineCaptureCount <> 2 then Halt(3);
	Write(output, labelText, ' ', Length(capturedTitle));
	for charIndex := 1 to Length(capturedTitle) do
		Write(output, ' ', Ord(capturedTitle[charIndex]));
	Write(output, ' ', lineCaptureCount);
	for lineIndex := 1 to lineCaptureCount do begin
		Write(output, ' ', Length(capturedLines[lineIndex]));
		for charIndex := 1 to Length(capturedLines[lineIndex]) do
			Write(output, ' ', Ord(capturedLines[lineIndex][charIndex]));
	end;
	WriteLn(output, ' ', titleWriteCount, ' ', Board.StatCount, ' ',
		Board.Tiles[10][10].Element, ' ', deltaX, ' ', deltaY);
end;

begin
	VideoWriteText := CaptureWrite;
	TextWindowInit(14, 4, 50, 16);
	TickSpeed := 4;
	New(IoTmpBuf);
	SoundEnabled := false;
	SoundBlockQueueing := false;
	SoundCurrentPriority := -1;
	SoundIsPlaying := false;
	SoundBuffer := '';
	Assign(output, 'SCROLLTH.OUT');
	Rewrite(output);
	RunCase('default', 'First line'#13'Second line'#13);
	RunCase('override', '@Scroll custom title'#13'First line'#13'Second line'#13);
	Close(output);
	Dispose(IoTmpBuf);
end.
