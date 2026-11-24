program MonitorProbe;
uses Crt, Video, GameVars, Input, Elements, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;

procedure RunCase(keyPressed: char; exitAlreadyRequested: boolean);
begin
	InputKeyPressed := keyPressed;
	GamePlayExitRequested := exitAlreadyRequested;
	ElementDefs[E_MONITOR].TickProc(0);
	WriteLn(output, Ord(keyPressed), ' ', Ord(GamePlayExitRequested), ' ', Ord(InputKeyPressed));
end;

begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	New(IoTmpBuf);
	WorldCreate;
	Assign(output, 'MONPRB.OUT');
	Rewrite(output);
	RunCase(#27, false);
	RunCase('a', false);
	RunCase('e', false);
	RunCase('h', false);
	RunCase('n', false);
	RunCase('p', false);
	RunCase('q', false);
	RunCase('r', false);
	RunCase('s', false);
	RunCase('w', false);
	RunCase('A', false);
	RunCase('E', false);
	RunCase('H', false);
	RunCase('N', false);
	RunCase('P', false);
	RunCase('Q', false);
	RunCase('R', false);
	RunCase('S', false);
	RunCase('W', false);
	RunCase('|', false);
	RunCase(' ', false);
	RunCase('b', false);
	RunCase('f', false);
	RunCase('x', false);
	RunCase('0', false);
	RunCase(#13, false);
	RunCase(Chr(200), false);
	RunCase('x', true);
	Close(output);
	Dispose(IoTmpBuf);
end.
