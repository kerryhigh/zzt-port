program OopParserProbe;
uses Crt, GameVars, Oop, Game;
var
	inputText: string[255];
	position, caseId, byteValue, charIndex: integer;
	lineData: array[0..300] of char;
	lineResult: string;
	lineLength: integer;
	output: text;

procedure SetInput(value: string);
begin
	inputText := value;
	Board.Stats[1].Data := @inputText[1];
	Board.Stats[1].DataLen := Length(inputText);
	position := 0;
end;

procedure SetLineInput(length: integer);
begin
	Board.Stats[1].Data := @lineData[0];
	Board.Stats[1].DataLen := length;
	position := 0;
end;

procedure ProbeWord(value: string);
begin
	SetInput(value);
	OopProbeReadWord(1, position);
	Inc(caseId);
	Write(output, 'W ', caseId, ' ', position, ' ', Length(OopWord));
	for charIndex := 1 to Length(OopWord) do
		Write(output, ' ', Ord(OopWord[charIndex]));
	WriteLn(output);
end;

procedure ProbeValue(value: string);
begin
	SetInput(value);
	OopValue := 1234;
	OopProbeReadValue(1, position);
	Inc(caseId);
	WriteLn(output, 'V ', caseId, ' ', OopValue, ' ', position);
end;

procedure ProbeLine;
begin
	lineResult := OopProbeReadLineToEnd(1, position);
	Inc(caseId);
	Write(output, 'L ', caseId, ' ', Length(lineResult), ' ', position);
	for charIndex := 1 to Length(lineResult) do
		Write(output, ' ', Ord(lineResult[charIndex]));
	WriteLn(output);
end;

begin
	New(IoTmpBuf);
	WorldCreate;
	BoardCreate;
	Board.StatCount := 1;
	Assign(output, 'OOPPARSE.OUT');
	Rewrite(output);

	caseId := 0;
	ProbeWord('7MOVE' + #13);
	ProbeWord('4ABC' + #13);
	ProbeWord('A1' + #13);
	ProbeWord('  NORTH' + #13);
	ProbeWord('_A:2' + #13);
	ProbeWord('ABCDEFGHIJKLMNOPQRSTUV' + #13);
	ProbeWord('');
	for byteValue := 0 to 255 do begin
		ProbeWord(Chr(byteValue));
		ProbeWord(' ' + Chr(byteValue) + 'A1!');
		ProbeWord('A' + Chr(byteValue) + 'B');
	end;

	caseId := 0;
	ProbeValue('');
	ProbeValue('x');
	ProbeValue('-9');
	ProbeValue(' 42,');
	ProbeValue('32767 ');
	ProbeValue('32768,');
	ProbeValue('65535?');
	ProbeValue('2147483647!');
	ProbeValue('2147483648!');
	ProbeValue('2147483649?');
	ProbeValue('99999999999999999999');
	ProbeValue('0000000000000000000042#');
	for byteValue := 0 to 255 do begin
		ProbeValue(Chr(byteValue));
		ProbeValue(' ' + Chr(byteValue) + '7!');
	end;

	caseId := 0;
	for lineLength := 0 to 255 do begin
		if lineLength > 0 then
			for charIndex := 0 to lineLength - 1 do
				lineData[charIndex] := 'A';
		lineData[lineLength] := #13;
		SetLineInput(lineLength + 1);
		ProbeLine;
	end;
	for lineLength := 0 to 255 do begin
		if lineLength > 0 then
			for charIndex := 0 to lineLength - 1 do
				lineData[charIndex] := 'A';
		SetLineInput(lineLength);
		ProbeLine;
	end;
	for byteValue := 0 to 255 do begin
		lineData[0] := Chr(byteValue);
		lineData[1] := #13;
		SetLineInput(2);
		ProbeLine;
	end;
	for byteValue := 0 to 255 do begin
		lineData[0] := Chr(byteValue);
		SetLineInput(1);
		ProbeLine;
	end;
	for byteValue := 0 to 255 do begin
		lineData[0] := 'A';
		lineData[1] := Chr(byteValue);
		lineData[2] := 'B';
		lineData[3] := #13;
		SetLineInput(4);
		ProbeLine;
	end;
	for byteValue := 0 to 255 do begin
		lineData[0] := 'A';
		lineData[1] := Chr(byteValue);
		lineData[2] := 'B';
		SetLineInput(3);
		ProbeLine;
	end;
	for charIndex := 0 to 299 do
		lineData[charIndex] := 'A';
	lineData[300] := #13;
	SetLineInput(301);
	ProbeLine;
	SetLineInput(300);
	ProbeLine;

	Close(output);
end.
