program SOUNDTIK;
uses Sounds;
var
	output: text;
	i: integer;
procedure StartPattern(pattern: string);
begin
	SoundEnabled := true;
	SoundBlockQueueing := false;
	SoundCurrentPriority := 1;
	SoundDurationMultiplier := 1;
	SoundDurationCounter := 1;
	SoundBuffer := pattern;
	SoundBufferPos := 1;
	SoundIsPlaying := true;
	SoundTimeCheckCounter := 0;
	TimerTicks := 0;
end;
begin
	Assign(output, 'SOUNDTIK.OUT');
	Rewrite(output);
	StartPattern(#0);
	Flush(output);
	SoundProbeTick;
	WriteLn(output, 'single ', TimerTicks, ' ', SoundBufferPos, ' ',
		SoundDurationCounter, ' ', Ord(SoundIsPlaying));
	Flush(output);

	StartPattern(#0#1#64);
	SoundProbeTick;
	WriteLn(output, 'odd-first ', TimerTicks, ' ', SoundBufferPos, ' ',
		SoundDurationCounter, ' ', Ord(SoundIsPlaying));
	Flush(output);
	SoundProbeTick;
	WriteLn(output, 'odd-end ', TimerTicks, ' ', SoundBufferPos, ' ',
		SoundDurationCounter, ' ', Ord(SoundIsPlaying));
	Flush(output);

	StartPattern(#0#1);
	SoundProbeTick;
	WriteLn(output, 'even-first ', TimerTicks, ' ', SoundBufferPos, ' ',
		SoundDurationCounter, ' ', Ord(SoundIsPlaying));
	Flush(output);
	SoundProbeTick;
	WriteLn(output, 'even-end ', TimerTicks, ' ', SoundBufferPos, ' ',
		SoundDurationCounter, ' ', Ord(SoundIsPlaying));
	Flush(output);

	{ A zero duration underflows the byte counter on the next timer tick. }
	StartPattern(#0#0#64#1);
	SoundProbeTick;
	WriteLn(output, 'zero-first ', TimerTicks, ' ', SoundBufferPos, ' ',
		SoundDurationCounter, ' ', Ord(SoundIsPlaying));
	Flush(output);
	for i := 1 to 255 do SoundProbeTick;
	WriteLn(output, 'zero-wrapped ', TimerTicks, ' ', SoundBufferPos, ' ',
		SoundDurationCounter, ' ', Ord(SoundIsPlaying));
	Flush(output);
	SoundProbeTick;
	WriteLn(output, 'zero-next ', TimerTicks, ' ', SoundBufferPos, ' ',
		SoundDurationCounter, ' ', Ord(SoundIsPlaying));
	Flush(output);
	Close(output);
end.
