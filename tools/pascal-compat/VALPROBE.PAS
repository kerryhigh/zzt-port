program ValProbe;
var
	value: integer;
	errorCode: integer;
	output: text;
begin
	Assign(output, 'VALPROBE.OUT');
	Rewrite(output);
	value := 123;
	Val('2147483647', value, errorCode);
	WriteLn(output, value, ' ', errorCode);
	value := 123;
	Val('2147483648', value, errorCode);
	WriteLn(output, value, ' ', errorCode);
	value := 123;
	Val('2147483649', value, errorCode);
	WriteLn(output, value, ' ', errorCode);
	value := 123;
	Val('99999999999999999999', value, errorCode);
	WriteLn(output, value, ' ', errorCode);
	Close(output);
end.
