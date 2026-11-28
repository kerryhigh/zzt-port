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

procedure RunWrappedBurstCase(labelText: string; keyBuffer: string; callCount: integer);
var
	callIndex: integer;
	remaining: string;
	remainingKey: integer;
begin
	InputProbeSetKeyBuffer(keyBuffer);
	for callIndex := 1 to callCount do begin
		InputUpdate;
		remaining := InputProbeGetKeyBuffer;
		remainingKey := 0;
		if Length(remaining) <> 0 then
			remainingKey := Ord(remaining[1]);
		WriteLn(output, labelText, ' ', Ord(InputKeyPressed), ' ', Length(remaining),
			' ', remainingKey, ' ', InputDeltaX, ' ', InputDeltaY);
	end;
end;

procedure RunLengthMatrix;
var
	keyLength: integer;
	keyIndex: integer;
	callIndex: integer;
	keyBuffer: string;
	remaining: string;
	remainingKey: integer;
begin
	Assign(output, 'INPUTMAT.OUT');
	Rewrite(output);
	for keyLength := 0 to 255 do begin
		keyBuffer := '';
		for keyIndex := 1 to keyLength do
			keyBuffer := keyBuffer + Chr(((keyIndex - 1) * 73 + 19) mod 255 + 1);
		InputKeyPressed := #0;
		InputProbeSetKeyBuffer(keyBuffer);
		for callIndex := 1 to 2 do begin
			InputUpdate;
			remaining := InputProbeGetKeyBuffer;
			remainingKey := 0;
			if Length(remaining) <> 0 then
				remainingKey := Ord(remaining[1]);
			WriteLn(output, 'matrix ', keyLength, ' ', callIndex, ' ',
				Ord(InputKeyPressed), ' ', Length(remaining), ' ', remainingKey,
				' ', InputDeltaX, ' ', InputDeltaY);
		end;
	end;
	Close(output);
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
	RunWrappedBurstCase('wrap-burst', 'ABCDEF', 2);
	Close(output);
	RunLengthMatrix;
end.
