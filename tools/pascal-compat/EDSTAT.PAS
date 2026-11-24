program EditorStatPasteProbe;
uses Crt, Video, GameVars, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
	template: TStat;
	modified: boolean;
	script: string[255];
	scriptBytes: array[0 .. 3] of byte;
	dataPtr: pointer;
	dataChar: char;
begin
	VideoWriteText := QuietWrite;
	New(IoTmpBuf);
	WorldCreate;
	BoardCreate;
	World.BoardCount := 1;
	World.Info.CurrentBoard := 0;
	Board.StatCount := 0;
	Board.Stats[0].X := 31;
	Board.Stats[0].Y := 12;
	Board.Tiles[31][12].Element := E_PLAYER;
	Board.Tiles[31][12].Color := $0F;

	template := StatTemplateDefault;
	template.Cycle := 9;
	template.StepX := 2;
	template.StepY := 3;
	template.P1 := 77;
	template.P2 := 88;
	template.P3 := 99;
	template.Follower := -1;
	template.Leader := -1;
	template.Data := nil;
	template.DataLen := 0;
	template.DataPos := 3;
	Board.Tiles[35][12].Element := E_SOLID;
	Board.Tiles[35][12].Color := $2F;
	modified := BoardPrepareTileForPlacement(35, 12);
	if modified then AddStat(35, 12, E_BOMB, $17, template.Cycle, template);

	Assign(output, 'EDSTAT.OUT');
	Rewrite(output);
	WriteLn(output, 'S ', Board.Tiles[35][12].Element, ' ',
		Board.Tiles[35][12].Color, ' ', Ord(modified), ' ', Board.StatCount, ' ',
		Board.Stats[1].X, ' ', Board.Stats[1].Y, ' ', Board.Stats[1].Cycle, ' ',
		(Board.Stats[1].StepX and $FF), ' ', (Board.Stats[1].StepY and $FF), ' ',
		Board.Stats[1].P1, ' ', Board.Stats[1].P2, ' ', Board.Stats[1].P3, ' ',
		(Board.Stats[1].Follower and $FF), ' ', (Board.Stats[1].Leader and $FF), ' ',
		Board.Stats[1].Under.Element, ' ', Board.Stats[1].Under.Color, ' ',
		Board.Stats[1].DataLen, ' ', Board.Stats[1].DataPos, ' ',
		0, ' ', 0, ' ', 0, ' ', 0, ' ', Ord(Board.Stats[1].Data <> nil));

	template := StatTemplateDefault;
	template.Cycle := 9;
	template.StepX := 2;
	template.StepY := 3;
	template.P1 := 77;
	template.P2 := 88;
	template.P3 := 99;
	template.Follower := -1;
	template.Leader := -1;
	script := 'A'#13#0'Z';
	Board.Stats[1].X := 31;
	Board.Stats[1].Y := 12;
	Board.Stats[1].Cycle := template.Cycle;
	Board.Stats[1].StepX := template.StepX;
	Board.Stats[1].StepY := template.StepY;
	Board.Stats[1].P1 := template.P1;
	Board.Stats[1].P2 := template.P2;
	Board.Stats[1].P3 := template.P3;
	Board.Stats[1].Follower := template.Follower;
	Board.Stats[1].Leader := template.Leader;
	Board.Stats[1].Data := @script[1];
	Board.Stats[1].DataLen := Length(script);
	Board.Stats[1].DataPos := 3;
	Board.Stats[1].Under.Element := E_SOLID;
	Board.Stats[1].Under.Color := $2F;
	Board.Tiles[31][12].Element := E_BOMB;
	Board.Tiles[31][12].Color := $17;
	template := Board.Stats[1];
	dataPtr := Board.Stats[1].Data;
	dataChar := 'B';
	Move(dataChar, dataPtr^, 1);
	Board.Tiles[36][12].Element := E_SOLID;
	Board.Tiles[36][12].Color := $2F;
	modified := BoardPrepareTileForPlacement(36, 12);
	if modified then AddStat(36, 12, E_BOMB, $17, template.Cycle, template);

	FillChar(scriptBytes, SizeOf(scriptBytes), 0);
	if Board.Stats[2].Data <> nil then
		Move(Board.Stats[2].Data^, scriptBytes, SizeOf(scriptBytes));
	WriteLn(output, 'D ', Board.Tiles[36][12].Element, ' ',
		Board.Tiles[36][12].Color, ' ', Ord(modified), ' ', Board.StatCount, ' ',
		Board.Stats[2].X, ' ', Board.Stats[2].Y, ' ', Board.Stats[2].Cycle, ' ',
		(Board.Stats[2].StepX and $FF), ' ', (Board.Stats[2].StepY and $FF), ' ',
		Board.Stats[2].P1, ' ', Board.Stats[2].P2, ' ', Board.Stats[2].P3, ' ',
		(Board.Stats[2].Follower and $FF), ' ', (Board.Stats[2].Leader and $FF), ' ',
		Board.Stats[2].Under.Element, ' ', Board.Stats[2].Under.Color, ' ',
		Board.Stats[2].DataLen, ' ', Board.Stats[2].DataPos, ' ',
		scriptBytes[0], ' ', scriptBytes[1], ' ', scriptBytes[2], ' ',
		scriptBytes[3], ' ', Ord(Board.Stats[2].Data <> nil));
	Close(output);
end.
