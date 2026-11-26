{$M 49152,163840,655360}
program MissingWorldReload;
uses Crt, Video, GameVars, Elements, Game, Input;

{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}

var
	output: text;
	loaded: boolean;
begin
	VideoWriteText := QuietWrite;
	New(IoTmpBuf);
	WorldCreate;
	World.Info.Name := 'MISSFILE';
	World.Info.IsSave := true;
	World.Info.Ammo := 17;
	World.Info.Gems := 23;
	World.Info.Health := 37;
	World.Info.Torches := 2;
	World.Info.TorchTicks := 123;
	World.Info.EnergizerTicks := 43;
	World.Info.Score := 777;
	ReturnBoardId := 77;
	Board.StatCount := 0;
	Board.Stats[0].X := 31;
	Board.Stats[0].Y := 11;
	Board.Stats[0].Cycle := 7;
	Board.Tiles[10][12].Element := E_SOLID;
	Board.Tiles[10][12].Color := $2E;

	{ Escape only acknowledges WorldLoad's missing-file error dialog. }
	InputProbeSetKeyBuffer('' + #27);
	loaded := WorldLoad(World.Info.Name, '.ZZT', false);
	ReturnBoardId := World.Info.CurrentBoard;

	Assign(output, 'MISSLOAD.OUT');
	Rewrite(output);
	WriteLn(output, Ord(loaded), ' ', ReturnBoardId, ' ', World.BoardCount,
		' ', World.Info.CurrentBoard, ' ', Ord(World.Info.IsSave),
		' ', World.Info.Ammo, ' ', World.Info.Gems, ' ', World.Info.Health,
		' ', World.Info.Torches, ' ', World.Info.TorchTicks,
		' ', World.Info.EnergizerTicks, ' ', World.Info.Score,
		' ', Board.StatCount, ' ', Board.Stats[0].X, ' ', Board.Stats[0].Y,
		' ', Board.Stats[0].Cycle, ' ', Board.Tiles[10][12].Element,
		' ', Board.Tiles[10][12].Color,
		' ', Ord(World.Info.Name = 'MISSFILE'));
	Close(output);

	WorldUnload;
	Dispose(IoTmpBuf);
end.
