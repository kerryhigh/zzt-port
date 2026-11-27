program BRDIOPRB;
{$I-}
type
	TBuffer = array[0..3] of byte;
	PBuffer = ^TBuffer;
var
	f: file;
	output: text;
	lengthWord: word;
	firstByte, secondByte: byte;
	payload: TBuffer;
	payloadCount: byte;
	readError, readAgainError, headerError, payloadError, payloadAgainError: integer;
	buffer: PBuffer;
begin
	Assign(f, 'PARTLEN.BRD');
	Rewrite(f, 1);
	Close(f);
	lengthWord := $1234;
	Assign(f, 'PARTLEN.BRD');
	Reset(f, 1);
	BlockRead(f, lengthWord, 2);
	readError := IOResult;
	readAgainError := IOResult;
	Close(f);
	Assign(output, 'BRDIOPRB.OUT');
	Rewrite(output);
	WriteLn(output, 'empty-length ', readError, ' ', lengthWord,
		' next-IOResult ', readAgainError);

	firstByte := $02;
	Assign(f, 'PARTLEN.BRD');
	Rewrite(f, 1);
	BlockWrite(f, firstByte, 1);
	Close(f);
	lengthWord := $1234;
	Assign(f, 'PARTLEN.BRD');
	Reset(f, 1);
	BlockRead(f, lengthWord, 2);
	readError := IOResult;
	readAgainError := IOResult;
	Close(f);
	WriteLn(output, 'partial-length ', readError, ' ', lengthWord,
		' next-IOResult ', readAgainError);

	payload[0] := $10;
	payload[1] := $20;
	payload[2] := $30;
	payload[3] := $40;
	for payloadCount := 0 to 3 do begin
		lengthWord := 4;
		Assign(f, 'SHORTPAY.BRD');
		Rewrite(f, 1);
		BlockWrite(f, lengthWord, 2);
		if payloadCount > 0 then
			BlockWrite(f, payload, payloadCount);
		Close(f);
		Assign(f, 'SHORTPAY.BRD');
		Reset(f, 1);
		BlockRead(f, lengthWord, 2);
		headerError := IOResult;
		GetMem(buffer, lengthWord);
		FillChar(buffer^, lengthWord, $A5);
		BlockRead(f, buffer^, lengthWord);
		payloadError := IOResult;
		payloadAgainError := IOResult;
		Close(f);
		WriteLn(output, 'short-payload ', payloadCount, ' ', headerError,
			' ', payloadError, ' next-IOResult ', payloadAgainError, ' ',
			buffer^[0], ' ', buffer^[1], ' ', buffer^[2], ' ', buffer^[3]);
		FreeMem(buffer, lengthWord);
	end;
	Close(output);
end.
