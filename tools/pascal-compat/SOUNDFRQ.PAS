program SoundFrequencyProbe;
uses Sounds;
var
	output: text;
	index: integer;
	octave: integer;
	note: integer;
begin
	Assign(output, 'SOUNDFRQ.OUT');
	Rewrite(output);
	index := 0;
	for octave := 1 to 6 do begin
		for note := 0 to 11 do begin
			if index > 0 then Write(output, ' ');
			Write(output, SoundFreqTable[octave * 16 + note]);
			Inc(index);
		end;
	end;
	WriteLn(output);
	Close(output);
end.
