program RandomVectorProbe;
const
	SeedCount = 16;
	RangeCount = 14;
var
	Seeds: array[1..SeedCount] of LongInt;
	Ranges: array[1..RangeCount] of Integer;
	seedIndex, rangeIndex, value: Integer;
	output: text;
begin
	Seeds[1] := 0;
	Seeds[2] := 1;
	Seeds[3] := 123;
	Seeds[4] := 2147483647;
	Seeds[5] := -2147483647 - 1;
	Seeds[6] := -1;
	Seeds[7] := -2147483647;
	Seeds[8] := 330;
	Seeds[9] := 26;
	Seeds[10] := 3;
	Seeds[11] := 85;
	Seeds[12] := 256;
	Seeds[13] := 454;
	Seeds[14] := -596792289;
	Seeds[15] := -1860696767;
	Seeds[16] := 134775814;

	Ranges[1] := 0;
	Ranges[2] := 1;
	Ranges[3] := 2;
	Ranges[4] := 3;
	Ranges[5] := 4;
	Ranges[6] := 7;
	Ranges[7] := 9;
	Ranges[8] := 10;
	Ranges[9] := 17;
	Ranges[10] := 100;
	Ranges[11] := 440;
	Ranges[12] := 1600;
	Ranges[13] := 5000;
	Ranges[14] := 32767;

	Assign(output, 'RNGVEC.OUT');
	Rewrite(output);
	for seedIndex := 1 to SeedCount do begin
		RandSeed := Seeds[seedIndex];
		for rangeIndex := 1 to RangeCount do begin
			value := Random(Ranges[rangeIndex]);
			WriteLn(output, seedIndex, ' ', rangeIndex, ' ', Ranges[rangeIndex],
				' ', value, ' ', (RandSeed and $FFFF), ' ', ((RandSeed shr 16) and $FFFF));
		end;
	end;
	Close(output);
end.
