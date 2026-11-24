program MessageTimerTickProbe;
uses Crt, Video, GameVars, Elements, Game;
var
	output: text;
	MessageRowChars: array[0..59] of byte;
	MessageRowColors: array[0..59] of byte;
{$F+}
procedure CaptureWrite(x, y, color: byte; text: TVideoLine);
var
	i, cell: integer;
begin
	if y = 24 then
		for i := 1 to Length(text) do begin
			cell := x + i - 1;
			if (cell >= 0) and (cell < 60) then begin
				MessageRowChars[cell] := Ord(text[i]);
				MessageRowColors[cell] := color;
			end;
		end;
end;
{$F-}

procedure ClearCapturedRow;
var
	i: integer;
begin
	for i := 0 to 59 do begin
		MessageRowChars[i] := 0;
		MessageRowColors[i] := 0;
	end;
end;

procedure SetupTimer(ticks: integer; messageText: string; currentStat: integer;
	timerX, timerY: integer; addLaterStat: boolean);
var
	timerStat: integer;
begin
	BoardCreate;
	ClearCapturedRow;
	TickTimeDuration := 0;
	DisplayMessage(ticks, messageText);
	timerStat := GetStatIdAt(0, 0);
	if timerStat < 1 then Halt(1);
	if (timerX <> 0) or (timerY <> 0) then begin
		Board.Tiles[Board.Stats[timerStat].X][Board.Stats[timerStat].Y]
			:= Board.Stats[timerStat].Under;
		Board.Stats[timerStat].X := timerX;
		Board.Stats[timerStat].Y := timerY;
		Board.Stats[timerStat].Under := Board.Tiles[timerX][timerY];
		Board.Tiles[timerX][timerY].Element := E_MESSAGE_TIMER;
		Board.Tiles[timerX][timerY].Color := 0;
	end;
	if addLaterStat then
		AddStat(20, 10, E_OBJECT, ElementDefs[E_OBJECT].Color, 1, StatTemplateDefault);
	CurrentStatTicked := currentStat;
end;

procedure WriteSnapshot(caseId: string; timerX, timerY: integer);
var
	timerStat, i, messageLength: integer;
begin
	timerStat := GetStatIdAt(timerX, timerY);
	if timerStat > 0 then
		Write(output, caseId, ' ', Board.StatCount, ' ', CurrentStatTicked, ' ',
			Length(Board.Info.Message), ' ', Board.Stats[timerStat].P2)
	else
		Write(output, caseId, ' ', Board.StatCount, ' ', CurrentStatTicked, ' ',
			Length(Board.Info.Message), ' -1');
	Write(output, ' ', Board.Tiles[0][0].Element, ' ', Board.Tiles[0][0].Color,
		' ', Board.Tiles[1][0].Element, ' ', Board.Tiles[1][0].Color);
	messageLength := Length(Board.Info.Message);
	for i := 1 to 10 do
		if i <= messageLength then
			Write(output, ' ', Ord(Board.Info.Message[i]))
		else
			Write(output, ' -1');
	for i := 0 to 59 do
		Write(output, ' ', MessageRowChars[i], ' ', MessageRowColors[i]);
	WriteLn(output);
	Flush(output);
end;

begin
	New(IoTmpBuf);
	VideoWriteText := CaptureWrite;
	TickSpeed := 4;
	WorldCreate;
	Assign(output, 'MSGTIK.OUT');
	Rewrite(output);

	SetupTimer(3, 'AB', 1, 0, 0, false);
	ElementDefs[E_MESSAGE_TIMER].TickProc(1);
	WriteSnapshot('hold', 0, 0);

	SetupTimer(1, 'LAST', 1, 0, 0, false);
	ElementDefs[E_MESSAGE_TIMER].TickProc(1);
	WriteSnapshot('expire-current', 0, 0);

	SetupTimer(1, 'MID', 2, 0, 0, true);
	ElementDefs[E_MESSAGE_TIMER].TickProc(1);
	WriteSnapshot('expire-before', 0, 0);

	SetupTimer(2, 'HOLD', 1, 1, 0, false);
	ElementDefs[E_MESSAGE_TIMER].TickProc(1);
	WriteSnapshot('nonzero-x', 1, 0);

	SetupTimer(-1, 'WRAP', 1, 0, 0, false);
	ElementDefs[E_MESSAGE_TIMER].TickProc(1);
	WriteSnapshot('byte-wrap', 0, 0);

	SetupTimer(2, 'SIDE', 1, 0, 1, false);
	ElementDefs[E_MESSAGE_TIMER].TickProc(1);
	WriteSnapshot('zero-x-side-y', 0, 1);

	SetupTimer(0, 'ZERO', 1, 0, 0, false);
	ElementDefs[E_MESSAGE_TIMER].TickProc(1);
	WriteSnapshot('zero-wrap', 0, 0);

	Close(output);
end.
