program DoorProbe;
uses Crt, Video, GameVars, Elements, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
	key, wrongKey, deltaX, deltaY: integer;
	targetHeld, wrongHeld: integer;

procedure RunBlackCase(gems, expectedElement, expectedGems: integer);
begin
	WorldCreate;
	World.Info.Gems := gems;
	Board.Tiles[10][10].Element := E_DOOR;
	Board.Tiles[10][10].Color := $00;
	deltaX := 0;
	deltaY := 0;
	ElementDefs[E_DOOR].TouchProc(10, 10, 0, deltaX, deltaY);
	if Board.Tiles[10][10].Element <> expectedElement then Halt(1);
	if World.Info.Gems <> expectedGems then Halt(2);
	WriteLn(output, 'black ', gems, ' ', Board.Tiles[10][10].Element, ' ', World.Info.Gems);
end;

procedure RunBlackKeyPickup;
begin
	WorldCreate;
	World.Info.Gems := 1;
	deltaX := 0;
	deltaY := 0;
	Board.Tiles[10][10].Element := E_KEY;
	Board.Tiles[10][10].Color := 0;
	ElementDefs[E_KEY].TouchProc(10, 10, 0, deltaX, deltaY);
	if World.Info.Gems <> 257 then Halt(9);
	if Board.Tiles[10][10].Element <> E_EMPTY then Halt(10);
	WriteLn(output, 'black-key pickup 1 ', World.Info.Gems, ' ', Board.Tiles[10][10].Element);

	Board.Tiles[11][10].Element := E_KEY;
	Board.Tiles[11][10].Color := 0;
	ElementDefs[E_KEY].TouchProc(11, 10, 0, deltaX, deltaY);
	if World.Info.Gems <> 257 then Halt(11);
	if Board.Tiles[11][10].Element <> E_KEY then Halt(12);
	WriteLn(output, 'black-key duplicate ', World.Info.Gems, ' ', Board.Tiles[11][10].Element);
end;

begin
	New(IoTmpBuf);
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	TickTimeDuration := 4;
	Assign(output, 'DOORPRB.OUT');
	Rewrite(output);

	RunBlackCase(1, E_DOOR, 1);
	RunBlackCase(257, E_EMPTY, 1);
	RunBlackCase(513, E_EMPTY, 1);
	RunBlackKeyPickup;

	for key := 1 to 7 do begin
		wrongKey := (key mod 7) + 1;
		WorldCreate;
		World.Info.Gems := 1;
		World.Info.Keys[wrongKey] := true;
		Board.Tiles[10][10].Element := E_DOOR;
		Board.Tiles[10][10].Color := key * 16;
		deltaX := 0;
		deltaY := 0;
		ElementDefs[E_DOOR].TouchProc(10, 10, 0, deltaX, deltaY);
		if Board.Tiles[10][10].Element <> E_DOOR then Halt(3);
		if not World.Info.Keys[wrongKey] then Halt(4);
		WriteLn(output, 'color ', key, ' wrong ', Board.Tiles[10][10].Element, ' ', World.Info.Gems);

		World.Info.Keys[key] := true;
		ElementDefs[E_DOOR].TouchProc(10, 10, 0, deltaX, deltaY);
		if Board.Tiles[10][10].Element <> E_EMPTY then Halt(5);
		if World.Info.Keys[key] then Halt(6);
		if not World.Info.Keys[wrongKey] then Halt(7);
		if World.Info.Gems <> 1 then Halt(8);
		targetHeld := 0;
		wrongHeld := 0;
		if World.Info.Keys[key] then targetHeld := 1;
		if World.Info.Keys[wrongKey] then wrongHeld := 1;
		WriteLn(output, 'color ', key, ' match ', Board.Tiles[10][10].Element, ' ',
			World.Info.Gems, ' ', targetHeld, ' ', wrongHeld);
	end;
	Close(output);
end.
