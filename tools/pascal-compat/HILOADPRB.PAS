program HiLoadProbe;
{$M 49152,163840,655360}
uses Crt, GameVars, Editor, Game, Input, Video, TxtWind;
const
	CaseNames: array[1 .. 8] of string[8] = (
		'HIMISS', 'HIEMPT', 'HIC001', 'HIC052',
		'HIC053', 'HIC1589', 'HIFULL', 'HILONG'
	);
var
	i: integer;
	seedFile: file of THighScoreList;
	seedList: THighScoreList;

procedure WriteScoreTable(filename: string);
var
	f: file of THighScoreList;
begin
	Assign(f, filename);
	Rewrite(f);
	Write(f, HighScoreList);
	Close(f);
end;

begin
	VideoInstall(80, Blue);
	TextWindowInit(5, 3, 50, 18);
	New(IoTmpBuf);
	VideoHideCursor;
	ClrScr;
	WorldCreate;
	Assign(seedFile, 'HISEED.HI');
	Reset(seedFile);
	Read(seedFile, seedList);
	Close(seedFile);
	for i := 1 to 8 do begin
		HighScoreList := seedList;
		World.Info.Name := CaseNames[i];
		HighScoresLoad;
		WriteScoreTable(World.Info.Name + '.OUT');
	end;
	Dispose(IoTmpBuf);
end.
