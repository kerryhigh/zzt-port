program WorldSaveProbe;
uses Crt, Dos, Video, Keys, Sounds, Input, TxtWind, GameVars, Elements, Editor, Oop, Game;
var
	signedValues: array[0..4] of integer;
	signedNames: array[0..4] of string[8];
	tag, i: integer;
	rawValue: byte;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
begin
	VideoWriteText := QuietWrite;
	New(IoTmpBuf);
	TickSpeed := 4;
	WorldCreate;
	if not WorldLoad('DEMO', '.ZZT', false) then Halt(1);
	World.Info.IsSave := true;
	WorldSave('PASOUT', '.SAV');

	{ Keep the active board valid while exercising every signed world-save
	  value in one direct Pascal session. }
	signedValues[0] := -32768;
	signedValues[1] := -1;
	signedValues[2] := 0;
	signedValues[3] := 1;
	signedValues[4] := 32767;
	signedNames[0] := 'WRS0';
	signedNames[1] := 'WRS1';
	signedNames[2] := 'WRS2';
	signedNames[3] := 'WRS3';
	signedNames[4] := 'WRS4';
	for tag := 0 to 4 do begin
		World.Info.Ammo := signedValues[tag];
		World.Info.Gems := signedValues[tag];
		World.Info.Health := signedValues[tag];
		World.Info.Torches := signedValues[tag];
		World.Info.TorchTicks := signedValues[tag];
		World.Info.EnergizerTicks := signedValues[tag];
		World.Info.Score := signedValues[tag];
		World.Info.BoardTimeSec := signedValues[tag];
		World.Info.BoardTimeHsec := signedValues[tag];
		WorldSave(signedNames[tag], '.SAV');
	end;

	{ Preserve raw Boolean representations and the 14-byte WorldInfo tail. }
	signedValues[0] := 0;
	signedValues[1] := 1;
	signedValues[2] := 2;
	signedValues[3] := 255;
	signedNames[0] := 'BLS0';
	signedNames[1] := 'BLS1';
	signedNames[2] := 'BLS2';
	signedNames[3] := 'BLS3';
	for tag := 0 to 3 do begin
		rawValue := signedValues[tag];
		for i := 1 to 7 do
			Move(rawValue, World.Info.Keys[i], SizeOf(rawValue));
		Move(rawValue, World.Info.IsSave, SizeOf(rawValue));
		for i := 0 to 13 do
			World.Info.unkPad[i] := signedValues[i mod 4];
		WorldSave(signedNames[tag], '.SAV');
	end;
end.
