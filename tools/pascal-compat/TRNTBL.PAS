program TransitionTableProbe;
uses Crt, Video, GameVars, Game;
var
	index: integer;
	output: text;
begin
	RandSeed := 0;
	GenerateTransitionTable;
	Assign(output, 'TRNTBL.OUT');
	Rewrite(output);
	for index := 1 to TransitionTableSize do
		WriteLn(output, index, ' ', TransitionTable[index].X, ' ', TransitionTable[index].Y);
	WriteLn(output, 'SEED ', (RandSeed and $FFFF), ' ', ((RandSeed shr 16) and $FFFF));
	Close(output);
end.
