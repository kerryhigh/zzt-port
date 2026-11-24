program BOARDDEC;
uses Crt, Video, GameVars, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
	fixture: array[0..511] of byte;
	p, i, remaining, count: integer;

procedure LoadFixture(zeroRun: boolean);
begin
	FillChar(fixture, SizeOf(fixture), 0);
	p := SizeOf(Board.Name);
	if zeroRun then begin
		fixture[p] := 0;
		fixture[p + 1] := 7;
		fixture[p + 2] := 8;
		Inc(p, 3);
		remaining := 1244;
		while remaining > 0 do begin
			if remaining > 255 then count := 255 else count := remaining;
			fixture[p] := count;
			fixture[p + 1] := 1;
			fixture[p + 2] := 2;
			Inc(p, 3);
			remaining := remaining - count;
		end;
	end else begin
		for i := 0 to 5 do begin
			fixture[p] := 255;
			fixture[p + 1] := i + 1;
			fixture[p + 2] := i + 11;
			Inc(p, 3);
		end;
	end;
	fixture[p] := 77;
	World.BoardCount := 0;
	World.Info.CurrentBoard := 0;
	World.BoardData[0] := @fixture;
	World.BoardLen[0] := SizeOf(fixture);
	BoardOpen(0);
end;

begin
	New(IoTmpBuf);
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	Assign(output, 'BOARDDEC.OUT');
	Rewrite(output);
	WorldCreate;

	LoadFixture(true);
	WriteLn(output, 'zero first ', Ord(Board.Tiles[1][1].Element), ' ', Board.Tiles[1][1].Color,
		' row-end ', Ord(Board.Tiles[60][1].Element), ' ', Board.Tiles[60][1].Color,
		' cell256 ', Ord(Board.Tiles[16][5].Element), ' ', Board.Tiles[16][5].Color,
		' cell257 ', Ord(Board.Tiles[17][5].Element), ' ', Board.Tiles[17][5].Color,
		' final ', Ord(Board.Tiles[60][25].Element), ' ', Board.Tiles[60][25].Color);

	LoadFixture(false);
	WriteLn(output, 'overrun cell1275 ', Ord(Board.Tiles[15][22].Element), ' ', Board.Tiles[15][22].Color,
		' cell1276 ', Ord(Board.Tiles[16][22].Element), ' ', Board.Tiles[16][22].Color,
		' final ', Ord(Board.Tiles[60][25].Element), ' ', Board.Tiles[60][25].Color,
		' max-shots ', Board.Info.MaxShots);
	Close(output);
end.
