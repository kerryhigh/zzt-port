program ElementDefinitionsProbe;
uses Crt, Video, GameVars, Elements, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
function EncodeHex(value: string): string;
const
	digits: string = '0123456789ABCDEF';
var
	i: integer;
	encoded: string;
begin
	if Length(value) = 0 then begin
		EncodeHex := '-';
		Exit;
	end;
	encoded := '';
	for i := 1 to Length(value) do
		encoded := encoded + digits[(Ord(value[i]) shr 4) + 1] +
			digits[(Ord(value[i]) and $0F) + 1];
	EncodeHex := encoded;
end;
var
	output: text;
	i: integer;
begin
	VideoWriteText := QuietWrite;
	New(IoTmpBuf);
	WorldCreate;
	Assign(output, 'ELDEFPRB.OUT');
	Rewrite(output);
	for i := 0 to MAX_ELEMENT do
		with ElementDefs[i] do
			WriteLn(output,
				i, ' ', Ord(Character), ' ', Color, ' ', Ord(Destructible), ' ',
				Ord(Pushable), ' ', Ord(VisibleInDark), ' ', Ord(PlaceableOnTop), ' ',
				Ord(Walkable), ' ', Ord(HasDrawProc), ' ', Cycle, ' ', EditorCategory, ' ',
				Ord(EditorShortcut), ' ', ScoreValue, ' ', Ord(Length(ParamTextName) <> 0), ' ',
				EncodeHex(Name), ' ', EncodeHex(CategoryName), ' ', EncodeHex(Param1Name), ' ',
				EncodeHex(Param2Name), ' ', EncodeHex(ParamBulletTypeName), ' ',
				EncodeHex(ParamBoardName), ' ', EncodeHex(ParamDirName), ' ',
				EncodeHex(ParamTextName));
	Close(output);
	Dispose(IoTmpBuf);
end.
