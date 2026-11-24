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
	readError, headerError, payloadError: integer;
	buffer: PBuffer;
begin
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
	Close(f);
	Assign(output, 'BRDIOPRB.OUT');
	Rewrite(output);
	WriteLn(output, 'partial-length ', readError, ' ', lengthWord);

	lengthWord := 4;
	firstByte := $10;
	secondByte := $20;
	Assign(f, 'SHORTPAY.BRD');
	Rewrite(f, 1);
	BlockWrite(f, lengthWord, 2);
	BlockWrite(f, firstByte, 1);
	BlockWrite(f, secondByte, 1);
	Close(f);
	Assign(f, 'SHORTPAY.BRD');
	Reset(f, 1);
	BlockRead(f, lengthWord, 2);
	headerError := IOResult;
	GetMem(buffer, lengthWord);
	FillChar(buffer^, lengthWord, $A5);
	BlockRead(f, buffer^, lengthWord);
	payloadError := IOResult;
	Close(f);
	WriteLn(output, 'short-payload ', headerError, ' ', payloadError, ' ',
		buffer^[0], ' ', buffer^[1], ' ', buffer^[2], ' ', buffer^[3]);
	Close(output);
	FreeMem(buffer, lengthWord);
end.
