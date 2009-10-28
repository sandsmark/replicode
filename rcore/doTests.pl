@tests = qw(add compare equality sub pointer red nowFAIL);
system("mkdir test_input") unless -d 'test_input';
system("mkdir test_output") unless -d 'test_output';
foreach $test (@tests) {

	system("perl assemble.pl examples/$test.rca > test_input/$test.rcode");
	system("perl assemble.pl output/$test.rca > test_output/$test.rcode");
	system("./testCore test_input/$test.rcode test_output/$test.rcode");
	my $status = ($? == 0) ? "PASS" : "FAIL";
	print "$test $status\n";
}
