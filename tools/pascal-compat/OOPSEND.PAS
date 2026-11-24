program OopSendProbe;
uses Crt, Video, GameVars, Oop, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
type
	TProbeScript = string[255];
var
	senderScript, lockedTargetScript, openTargetScript: TProbeScript;
	output: text;
	sent: boolean;
	targetStartPosition: integer;

procedure InstallScript(statId: integer; var source: TProbeScript);
begin
	GetMem(Board.Stats[statId].Data, Length(source));
	Move(source[1], Board.Stats[statId].Data^, Length(source));
	Board.Stats[statId].DataLen := Length(source);
	Board.Stats[statId].DataPos := 0;
end;

begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	New(IoTmpBuf);
	WorldCreate;
	Assign(output, 'OOPSEND.OUT');
	Rewrite(output);

	{ A positive object send can enter its own locked label. }
	Board.StatCount := 1;
	senderScript := '@SENDER' + #13 + ':GO' + #13 + '#END' + #13;
	InstallScript(1, senderScript);
	Board.Stats[1].P2 := 1;
	sent := OopSend(1, 'GO', false);
	WriteLn(output, 'S ', Ord(sent), ' ', Board.Stats[1].DataPos);

	{ A negative in-world event respects the same object's lock. }
	Board.Stats[1].DataPos := 0;
	sent := OopSend(-1, 'GO', false);
	WriteLn(output, 'E ', Ord(sent), ' ', Board.Stats[1].DataPos);

	{ OopReadWord uppercases the command token; object and label source text may be lowercase. }
	FillChar(Board.Stats, SizeOf(Board.Stats), 0);
	Board.StatCount := 3;
	senderScript := '@SENDER' + #13;
	lockedTargetScript := '@VILLAIN' + #13 + ':HELLO' + #13;
	openTargetScript := '@villain' + #13 + ':hello' + #13;
	InstallScript(1, senderScript);
	InstallScript(2, lockedTargetScript);
	InstallScript(3, openTargetScript);
	Board.Stats[2].P2 := 1;
	sent := OopSend(1, 'VILLAIN:HELLO', false);
	WriteLn(output, 'X ', Ord(sent), ' ', Board.Stats[2].DataPos, ' ', Board.Stats[3].DataPos);

	{ The game passes DataPos by reference; a self-send redirects this call. }
	FillChar(Board.Stats, SizeOf(Board.Stats), 0);
	Board.StatCount := 1;
	senderScript := '#SEND GO' + #13 + '#CYCLE 13' + #13 + '#END' + #13 +
		':GO' + #13 + '#CYCLE 17' + #13 + '#END' + #13;
	InstallScript(1, senderScript);
	Board.Stats[1].P2 := 1;
	OopExecute(1, Board.Stats[1].DataPos, 'Self send dispatch');
	WriteLn(output, 'D ', Board.Stats[1].Cycle);

	{ Parsed cross-object send continues locally; its target starts at the label. }
	FillChar(Board.Stats, SizeOf(Board.Stats), 0);
	Board.StatCount := 3;
	senderScript := '#SEND VILLAIN:HELLO' + #13 + '#CYCLE 5' + #13 + '#END' + #13;
	lockedTargetScript := '@VILLAIN' + #13 + ':HELLO' + #13 + '#CYCLE 3' + #13 + '#END' + #13;
	openTargetScript := '@villain' + #13 + ':hello' + #13 + '#CYCLE 9' + #13 + '#END' + #13;
	InstallScript(1, senderScript);
	InstallScript(2, lockedTargetScript);
	InstallScript(3, openTargetScript);
	Board.Stats[2].P2 := 1;
	OopExecute(1, Board.Stats[1].DataPos, 'Cross-object send dispatch');
	targetStartPosition := Board.Stats[3].DataPos;
	OopExecute(3, Board.Stats[3].DataPos, 'Villain target');
	WriteLn(output, 'Q ', Board.Stats[1].Cycle, ' ', Board.Stats[2].DataPos,
		' ', targetStartPosition, ' ', Board.Stats[3].DataPos, ' ', Board.Stats[3].Cycle);
	Close(output);
end.
