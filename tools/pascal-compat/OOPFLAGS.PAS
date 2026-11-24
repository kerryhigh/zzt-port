program OopFlagsProbe;
uses Crt, Video, GameVars, Oop, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
type
	TProbeScript = string[255];
var
	i, j, position: integer;
	script: TProbeScript;
	output: text;

procedure WriteFlagMap(prefix: char);
var
	i, j: integer;
begin
	for i := 1 to MAX_FLAG do begin
		Write(output, prefix, ' ', i, ' ', Ord(World.Info.Flags[i][0]));
		for j := 1 to 20 do
			Write(output, ' ', Ord(World.Info.Flags[i][j]));
		WriteLn(output);
	end;
end;

procedure FillFlagTail(marker: byte);
var
	i, j: integer;
begin
	for i := 1 to MAX_FLAG do begin
		World.Info.Flags[i][0] := #0;
		for j := 1 to 20 do
			World.Info.Flags[i][j] := Chr(marker + i);
	end;
end;

procedure InstallScript(var source: TProbeScript);
begin
	GetMem(Board.Stats[1].Data, Length(source));
	Move(source[1], Board.Stats[1].Data^, Length(source));
	Board.Stats[1].DataLen := Length(source);
	Board.Stats[1].DataPos := 0;
end;

begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	New(IoTmpBuf);
	WorldCreate;
	Board.StatCount := 1;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].Cycle := 3;
	Board.Tiles[10][10].Element := E_OBJECT;
	Board.Tiles[10][10].Color := $0F;

	FillFlagTail($A0);
	World.Info.Flags[1] := 'ALPHA';
	World.Info.Flags[2] := 'DUP';
	World.Info.Flags[3] := 'DUP';
	World.Info.Flags[4] := 'TAIL';
	script := '#CLEAR DUP' + #13 + '#CLEAR DUP' + #13 +
		'#SET BETA' + #13 + '#SET alpha' + #13 + '#END' + #13;
	InstallScript(script);
	position := 0;
	OopExecute(1, position, 'Flag map probe');
	if (Board.Info.Message <> '') or (Board.Stats[1].DataPos < 0) then
		Halt(2);
	Assign(output, 'OOPFLAGS.OUT');
	Rewrite(output);
	WriteFlagMap('A');
	WriteLn(output, 'I ', WorldGetFlagPosition('ALPHA'), ' ',
		WorldGetFlagPosition('DUP'), ' ', WorldGetFlagPosition('BETA'), ' ',
		WorldGetFlagPosition('MISSING'));

	FreeMem(Board.Stats[1].Data, Board.Stats[1].DataLen);
	FillFlagTail($B0);
	World.Info.Flags[1] := 'F1';
	World.Info.Flags[2] := 'F2';
	World.Info.Flags[3] := 'F3';
	World.Info.Flags[4] := 'F4';
	World.Info.Flags[5] := 'F5';
	World.Info.Flags[6] := 'F6';
	World.Info.Flags[7] := 'F7';
	World.Info.Flags[8] := 'F8';
	World.Info.Flags[9] := 'F9';
	World.Info.Flags[10] := 'F10';
	script := '#SET OVERFLOW' + #13 + '#END' + #13;
	InstallScript(script);
	position := 0;
	OopExecute(1, position, 'Full flag table probe');
	if (Board.Info.Message <> '') or (Board.Stats[1].DataPos < 0) then
		Halt(3);
	WriteFlagMap('F');
	WriteLn(output, 'J ', WorldGetFlagPosition('F1'), ' ',
		WorldGetFlagPosition('F10'), ' ', WorldGetFlagPosition('OVERFLOW'));
	Close(output);
end.
