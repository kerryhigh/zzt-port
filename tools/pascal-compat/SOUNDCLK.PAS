program SoundClockProbe;
uses Crt, Sounds;
var
	output: text;
	counter: integer;
	elapsed: boolean;

procedure RunCase(labelText: string; hour, minute, sec, hSec: word;
	initialCounter, duration: integer);
begin
	SoundProbeSetTime(hour, minute, sec, hSec);
	counter := initialCounter;
	elapsed := SoundHasTimeElapsed(counter, duration);
	WriteLn(output, labelText, ' ', Ord(elapsed), ' ', counter);
end;

begin
	UseSystemTimeForElapsed := true;
	SoundTimeCheckCounter := 0;
	Assign(output, 'SOUNDCLK.OUT');
	Rewrite(output);
	RunCase('before-minute-wrap', 12, 34, 59, 99, 5990, 10);
	RunCase('last-centisecond', 12, 34, 59, 99, 5990, 9);
	RunCase('minute-rollover', 12, 35, 0, 1, 5999, 2);
	RunCase('below-threshold', 12, 35, 0, 5, 1, 5);
	RunCase('at-threshold', 12, 35, 0, 6, 1, 5);
	RunCase('zero-duration', 12, 35, 0, 1, 1, 0);
	RunCase('backward-clock-step', 12, 35, 0, 90, 100, 25);
	Close(output);
	SoundUninstall;
end.
