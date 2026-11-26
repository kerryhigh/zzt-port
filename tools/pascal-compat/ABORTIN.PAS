{$M 49152,163840,655360}
program EndPlayDecisionReplay;
uses Crt, Video, GameVars, Elements, Game, Input;

var
	output: text;
	titleOutput: text;

procedure CapturePromptCase(caseId: integer; answer: string; wasPaused: boolean);
	begin
		World.Info.Health := 37;
		GameStateElement := E_PLAYER;
		GamePaused := wasPaused;
		GamePlayExitRequested := false;
		InputKeyPressed := 'Q';
		InputProbeSetKeyBuffer('');
		InputProbeSetReadKeys(answer);
		GamePromptEndPlay;
		WriteLn(output, caseId, ' ', Ord(GamePlayExitRequested), ' ',
			Ord(InputKeyPressed), ' ', Ord(GamePaused), ' ', World.Info.Health);
	end;

procedure CaptureTitleQuitCase(caseId: integer; answer: string);
	begin
		GameTitleExitRequested := false;
		InputKeyPressed := 'Q';
		InputProbeSetKeyBuffer('');
		InputProbeSetReadKeys(answer);
		GameTitleExitRequested := SidebarPromptYesNo('Quit ZZT? ', true);
		WriteLn(titleOutput, caseId, ' ', Ord(GameTitleExitRequested));
	end;

begin
	New(IoTmpBuf);
	RandSeed := 0;
	WorldCreate;
	Assign(output, 'ABORT.OUT');
	Rewrite(output);
	CapturePromptCase(1, 'Y', false);
	CapturePromptCase(2, 'N', false);
	CapturePromptCase(3, #27, false);
	CapturePromptCase(4, 'Y', true);
	CapturePromptCase(5, 'N', true);
	CapturePromptCase(6, #27, true);
	World.Info.Health := 0;
	GameStateElement := E_PLAYER;
	GamePaused := false;
	GamePlayExitRequested := false;
	InputKeyPressed := 'Q';
	GamePromptEndPlay;
	WriteLn(output, 7, ' ', Ord(GamePlayExitRequested), ' ',
		Ord(InputKeyPressed), ' ', Ord(GamePaused), ' ', World.Info.Health);
	Close(output);
	Assign(titleOutput, 'TQUIT.OUT');
	Rewrite(titleOutput);
	CaptureTitleQuitCase(1, 'Y');
	CaptureTitleQuitCase(2, 'N');
	CaptureTitleQuitCase(3, #27);
	Close(titleOutput);
	Dispose(IoTmpBuf);
end.
