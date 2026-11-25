program BoardNeighborPassageReplay;
uses Crt, Video, GameVars, Elements, Sounds, Game, Input;
var
	cellScreenValid: array[0..59, 0..24] of boolean;
	cellScreenColors, cellScreenCharacters: array[0..59, 0..24] of byte;
	idempotentCellWriteCount: longint;
{$F+}
procedure CaptureCellWrite(x, y, color: byte; text: TVideoLine);
var
	charIndex, cellX: integer;
	eventIndex: longint;
begin
	if not GameLoopProbeCellCaptureEnabled then Exit;
	for charIndex := 1 to Length(text) do begin
		cellX := x + charIndex - 1;
		if (cellX >= 0) and (cellX < 60) and (y < 25) then begin
			if cellScreenValid[cellX, y]
				and (cellScreenColors[cellX, y] = color)
				and (cellScreenCharacters[cellX, y] = Ord(text[charIndex])) then
				Inc(idempotentCellWriteCount)
			else begin
				cellScreenValid[cellX, y] := true;
				cellScreenColors[cellX, y] := color;
				cellScreenCharacters[cellX, y] := Ord(text[charIndex]);
				if (GameLoopProbeCellWriteEvents <> nil) and
					(GameLoopProbeCellWriteCount < GameLoopProbeCellWriteCapacity) then begin
					eventIndex := GameLoopProbeCellWriteCount + 1;
					GameLoopProbeCellWriteEvents^[eventIndex].X := cellX;
					GameLoopProbeCellWriteEvents^[eventIndex].Y := y;
					GameLoopProbeCellWriteEvents^[eventIndex].Color := color;
					GameLoopProbeCellWriteEvents^[eventIndex].Character := Ord(text[charIndex]);
					GameLoopProbeCellWriteCount := eventIndex;
				end else
					GameLoopProbeCellWriteOverflow := true;
			end;
		end;
	end;
end;
{$F-}

procedure WriteCellWriteEvents(filename: string);
var
	cellOutput: text;
	eventIndex: longint;
begin
	Assign(cellOutput, filename);
	Rewrite(cellOutput);
	WriteLn(cellOutput, GameLoopProbeCellWriteCount, ' ', Ord(GameLoopProbeCellWriteOverflow), ' ', idempotentCellWriteCount);
	for eventIndex := 1 to GameLoopProbeCellWriteCount do begin
		with GameLoopProbeCellWriteEvents^[eventIndex] do
			WriteLn(cellOutput, X, ' ', Y, ' ', Color, ' ', Character);
	end;
	Close(cellOutput);
end;

var
	output: text;
	loadOutput: text;
	roundTripLoaded: boolean;
	statId, boardId, passageStatId: integer;

begin
	VideoWriteText := CaptureCellWrite;
	New(IoTmpBuf);
	RandSeed := 0;
	{ Match ZZT.PAS startup: shuffle the transition table before WorldCreate. }
	GenerateTransitionTable;
	WorldCreate;
	{ WorldCreate leaves this word undefined; match the controlled production allocation. }
	World.Info.unk1 := 0;
	BoardCreate;
	World.BoardCount := 2;
	for boardId := 1 to 2 do begin
		World.BoardData[boardId] := nil;
		World.BoardLen[boardId] := 0;
	end;
	World.Info.Health := 100;
	World.Info.Ammo := 0;
	World.Info.Gems := 0;
	World.Info.Score := 0;

	{ Board 0 exits west into board 1 at the matching passage tile. }
	Board.Tiles[30][12].Element := E_EMPTY;
	Board.Tiles[30][12].Color := 0;
	Board.Tiles[1][5].Element := E_EMPTY;
	Board.Tiles[1][5].Color := 0;
	Board.Stats[0].X := 2;
	Board.Stats[0].Y := 5;
	Board.Stats[0].StepX := 0;
	Board.Stats[0].StepY := 0;
	Board.Stats[0].Cycle := 1;
	Board.Stats[0].Under.Element := E_EMPTY;
	Board.Stats[0].Under.Color := 0;
	Board.Tiles[2][5].Element := E_PLAYER;
	Board.Tiles[2][5].Color := ElementDefs[E_PLAYER].Color;
	Board.Info.NeighborBoards[2] := 1;
	BoardClose;

	{ Board 1 starts at its west entry edge; the passage is one step inward. }
	World.Info.CurrentBoard := 1;
	BoardCreate;
	Board.Tiles[30][12].Element := E_EMPTY;
	Board.Tiles[30][12].Color := 0;
	Board.Stats[0].X := 60;
	Board.Stats[0].Y := 5;
	Board.Stats[0].StepX := 0;
	Board.Stats[0].StepY := 0;
	Board.Stats[0].Under.Element := E_EMPTY;
	Board.Stats[0].Under.Color := 0;
	Board.Tiles[60][5].Element := E_PLAYER;
	Board.Tiles[60][5].Color := ElementDefs[E_PLAYER].Color;
	AddStat(59, 5, E_PASSAGE, $2A, 0, StatTemplateDefault);
	passageStatId := Board.StatCount;
	Board.Stats[passageStatId].P3 := 2;
	BoardClose;

	{ Two color-matched destinations verify the source's last-match scan. }
	World.Info.CurrentBoard := 2;
	BoardCreate;
	Board.Tiles[23][17].Element := E_EMPTY;
	Board.Tiles[23][17].Color := 0;
	AddStat(20, 15, E_PASSAGE, $2A, 0, StatTemplateDefault);
	AddStat(22, 17, E_PASSAGE, $2A, 0, StatTemplateDefault);
	BoardClose;
	BoardChange(0);

	GameStateElement := E_PLAYER;
	GamePaused := true;
	JustStarted := false;
	TickSpeed := 0;
	TickTimeCounter := 0;
	SoundTimeCheckCounter := 0;
	UseSystemTimeForElapsed := true;
	SoundEnabled := false;
	SoundIsPlaying := false;
	SoundCurrentPriority := -1;
	SoundBlockQueueing := false;
	SoundBuffer := '';
	SoundBufferPos := 1;
	SoundDurationCounter := 1;
	SoundDurationMultiplier := 1;
	InputDeltaX := 0;
	InputDeltaY := 0;
	InputKeyPressed := #0;
	InputShiftPressed := false;
	InputShiftAccepted := false;
	PlayerDirX := 0;
	PlayerDirY := 0;
	CurrentTick := 0;
	CurrentStatTicked := 0;
	TickTimeDuration := 0;
	GamePlayExitRequested := false;
	GameLoopProbeCellCaptureEnabled := false;
	GameLoopProbeCellWriteCount := 0;
	GameLoopProbeCellWriteOverflow := false;
	FillChar(cellScreenValid, SizeOf(cellScreenValid), 0);
	FillChar(cellScreenColors, SizeOf(cellScreenColors), 0);
	FillChar(cellScreenCharacters, SizeOf(cellScreenCharacters), 0);
	idempotentCellWriteCount := 0;
	GetMem(GameLoopProbeCellWriteEvents, SizeOf(TGameLoopProbeCellWriteEventArray));

	Assign(output, 'INTEGPR.OUT');
	Rewrite(output);
	Close(output);
	{ pauseBlink is undefined in original GamePlayLoop; seed it for this deterministic fixture. }
	GameLoopProbeSeedPauseBlink := true;
	GameLoopProbeEnabled := true;
	GameLoopProbeCapture('L', 0);
	InputProbeSetNormalizedTape('LLLR');
	GameLoopProbeCaptureNextBoardChange := true;
	GameLoopProbeStopAfter := 6;
	GameLoopProbeCycles := 0;
	{ A fixed PIT-equivalent clock avoids poll-count differences between the
	  Pascal loop and LibZoo's incremental tick state machine. }
	SoundProbeSetTime(0, 0, 0, 30);
	SoundProbeQueueCapture(false);
	SoundProbeQueueCapture(true);
	GamePlayLoop(false);
	GameLoopProbeCellCaptureEnabled := false;
	GameLoopProbeEnabled := false;
	if InputProbeGetNormalizedTape <> '' then RunError(254);
	SoundProbeQueueCapture(false);
	SoundProbeWriteQueueEvents('INTEGSND.OUT');
	WriteCellWriteEvents('INTEGCEL.OUT');
	WorldSave('INTEGSAV', '.SAV');
	GameLoopProbeCapture('T', 2);
	roundTripLoaded := WorldLoad('INTEGSAV', '.SAV', false);
	Assign(loadOutput, 'INTEGLOD.OUT');
	Rewrite(loadOutput);
	WriteLn(loadOutput, Ord(roundTripLoaded));
	Close(loadOutput);
	if not roundTripLoaded then RunError(255);
	GameLoopProbeCapture('T', 3);
	WorldSave('INTEG2SV', '.SAV');
	GameLoopProbeCapture('F', 0);
	FreeMem(GameLoopProbeCellWriteEvents, SizeOf(TGameLoopProbeCellWriteEventArray));
	GameLoopProbeCellWriteEvents := nil;
	WorldUnload;
	Dispose(IoTmpBuf);
end.
