program HiSaveProbe;
{$M 49152,163840,655360}
uses Crt, GameVars, Editor, Game, Input, Video, TxtWind;
var
	i: integer;
	longName: string[50];

procedure WriteScoreTable(filename: string);
	var
		f: file of THighScoreList;
	begin
		Assign(f, filename);
		Rewrite(f);
		Write(f, HighScoreList);
		Close(f);
	end;

procedure PrepareScoreTable;
	var
		i, j: integer;
		longName: string[50];
	begin
		for i := 1 to HIGH_SCORE_COUNT do begin
			longName := '';
			for j := 1 to 50 do
				longName := longName + Chr(65 + ((i + j) mod 26));
			HighScoreList[i].Name := longName;
			HighScoreList[i].Name := 'R';
			HighScoreList[i].Name := HighScoreList[i].Name + Chr(65 + ((i - 1) mod 26));
			HighScoreList[i].Score := 30000 - ((i - 1) * 500);
		end;
		{ Two equal entries verify that HighScoresAdd inserts before the first tie. }
		HighScoreList[16].Score := HighScoreList[15].Score;
	end;

procedure RunAddCase(caseName: TString50; score: integer; promptKeys: string);
	begin
		PrepareScoreTable;
		WriteScoreTable(caseName + '.IN');
		World.Info.Name := caseName;
		if Length(promptKeys) = 0 then
			InputProbeSetKeyBuffer(#13)
		else
			InputProbeSetReadKeys(promptKeys);
		HighScoresAdd(score);
		WriteScoreTable(caseName + '.OUT');
	end;
begin
	VideoInstall(80, Blue);
	TextWindowInit(5, 3, 50, 18);
	New(IoTmpBuf);
	VideoHideCursor;
	ClrScr;
	TickSpeed := 4;
	GenerateTransitionTable;
	WorldCreate;
	World.Info.Name := 'PASPROBE';
	for i := 1 to HIGH_SCORE_COUNT do begin
		HighScoreList[i].Name := '';
		HighScoreList[i].Score := -1;
	end;
	longName := '';
	for i := 1 to 50 do
		longName := longName + 'A';
	HighScoreList[1].Name := longName;
	HighScoreList[1].Name := 'X';
	HighScoreList[1].Score := -32768;
	HighScoreList[2].Name := #0 + #130 + 'X';
	HighScoreList[2].Score := 32767;
	HighScoresSave;

	RunAddCase('HITOP', 32767, '');
	RunAddCase('HITIE', 23000, '');
	RunAddCase('HIBOT', 15500, '');
	RunAddCase('HICUT', 15499, '');
	RunAddCase('HIZERO', 0, '');
	RunAddCase('HINEG', -32768, '');
	RunAddCase('HINAME', 25000, 'KERRYY' + #8 + #13);
	Dispose(IoTmpBuf);
end.
