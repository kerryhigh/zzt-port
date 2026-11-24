program InputBufferProbe;
uses Crt, Input;
var
	output: text;

procedure RunCase(labelText: string; keyBuffer: string; callCount: integer);
var
	callIndex: integer;
begin
	InputProbeSetKeyBuffer(keyBuffer);
	for callIndex := 1 to callCount do begin
		InputUpdate;
		WriteLn(output, labelText, ' ', Ord(InputKeyPressed));
	end;
end;

procedure RunDirectionCase(labelText: string; key: char);
begin
	InputProbeSetKeyBuffer(key);
	InputUpdate;
	WriteLn(output, labelText, ' ', Ord(InputKeyPressed), ' ', InputDeltaX, ' ', InputDeltaY,
		' ', InputLastDeltaX, ' ', InputLastDeltaY);
end;

procedure RunDirectionBurstCase(labelText: string; key: char; callCount: integer);
var
	callIndex: integer;
	keyBuffer: string;
begin
	InputLastDeltaX := 0;
	InputLastDeltaY := 0;
	keyBuffer := '';
	keyBuffer := keyBuffer + key;
	keyBuffer := keyBuffer + key;
	InputProbeSetKeyBuffer(keyBuffer);
	for callIndex := 1 to callCount do begin
		InputUpdate;
		WriteLn(output, labelText, ' ', Ord(InputKeyPressed), ' ', InputDeltaX, ' ', InputDeltaY,
			' ', InputLastDeltaX, ' ', InputLastDeltaY);
	end;
end;

begin
	InputJoystickEnabled := false;
	InputMouseEnabled := false;
	InputShiftAccepted := false;
	InputLastDeltaX := 0;
	InputLastDeltaY := 0;
	Assign(output, 'INPUTBUF.OUT');
	Rewrite(output);
	RunCase('one', 'A', 2);
	RunCase('two', 'AB', 3);
	RunCase('three', 'ABC', 3);
	RunCase('four', 'ABCD', 3);
	RunCase('repeat-four', 'RRRR', 3);
	RunCase('cp437-high', Chr(130), 1);
	RunCase('cp437-top', Chr(255), 1);
	RunDirectionCase('arrow-up', KEY_UP);
	RunDirectionCase('arrow-left', KEY_LEFT);
	RunDirectionCase('arrow-right', KEY_RIGHT);
	RunDirectionCase('arrow-down', KEY_DOWN);
	RunDirectionBurstCase('arrow-right-burst', KEY_RIGHT, 3);
	RunDirectionCase('numeric-up', '8');
	RunDirectionCase('numeric-left', '4');
	RunDirectionCase('numeric-right', '6');
	RunDirectionCase('numeric-down', '2');
	RunDirectionCase('neutral', '5');
	InputProbeSetKeyBuffer('');
	InputUpdate;
	WriteLn(output, 'empty ', Ord(InputKeyPressed), ' ', InputDeltaX, ' ', InputDeltaY,
		' ', InputLastDeltaX, ' ', InputLastDeltaY);
	Close(output);
end.
