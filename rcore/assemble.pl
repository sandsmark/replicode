%operatorReturnType = (
	'' => 0,
	'[]' => 1,
	'nb' => 2,
	'bl' => 3,
	'ms' => 4,
	'st' => 5,
	'sid' => 6,
	'did' => 7,
	'fid' => 8
);

%atomType = (
	'nil' => 0x80,
	':' => 0x82,
	'::' => 0x83,
	'iptr' => 0x84,
	'view' => 0x85,
	'vptr' => 0x86,
	'this' => 0x87,
	'entity' => 0x88,
	'sid' => 0x89,
	'did' => 0x8A,
	'undefined' => 0x8F,
	'cptr' => 0xC0,
	'object' => 0xC2,
	'timestamp' => 0xC6,
	'wildcard' => 0x82
);

sub replace {
	my ($seq) = @_;
	my @words = split(/\s+/, $seq);
	my $orig = $seq;
	for ($i = 0; $i <= $#words; ++$i) {
		my $word = $words[$i];
		if (defined $definition{$word}) {
			$words[$i] = $definition{$word};
		} elsif (defined $count{$word}) {
			my $b = sprintf("0x%04x", $count{$word});
			$words[$i] = $b;
			++$count{$word};
		}
	}
	$bits = join(" ", @words);
	$bits =~ s/0x([\da-fA-F]) /0x0\1 /g;
	$bits =~ s/0x([\da-fA-F])$/0x0\1/g;
	$bits =~ s/0x//g;
	$bits =~ s/ //g;
	$bits;
#	join(" ", @words);
}

sub replaceClass
{
	my ($dev, $match, $tpl, @args) = @_;
	unless (defined $class{$tpl}) {
		warn "no template for $tpl in '$dev'";
		return $dev;
	}

	my $replacement = $class{$tpl};
	while ($replacement =~ /:\w+/) {
		$t = shift @args;
		$t =~ s/:/#/g;
		$replacement =~ s/:\w+/$t/;
	}
	$dev =~ s/\($match\)/$replacement/;
	$dev =~ s/#/:/g;
	$dev =~ s/ nil//g;
	$dev;
}

open(OPCODES, "<std.replicode");
open(OPCODES_H_TMP, ">opcodes.h.tmp");
while (<OPCODES>) {
	chomp;
	s/<(.+)>/\1/;
	s/;.*//;
	next if /^$/;
	if (/!def\s+([-\w|<>=+*\\]+)\s+(.*)/) {
		my ($name, $def) = ($1, $2);
		$rep = $2;
		$rep =~ s/{(.+)}/\1/;
		$rep = &replace($rep);
		$definition{$name} = $rep;
#		print "$name := $rep\n";
	} elsif (/def\s+\(/) {
		next;
	} elsif (/!counter\s+(\w+)\s+(\d+)/) {
		$count{$1} = $2;
#!class (_react_obj (_obj {fun act tsc csm sig clk nfy :x} nil))
	} elsif (/!class\s+\(([\w\.|]+)(\[\])?\s+(.*)\)/) {
		my ($class, $set, $dev) = ($1, $2, $3);
		$dev =~ s/:\[\]/=SET/g;
		if ($dev =~ /([{\[].*[}\]])/) {
			$hack = $1;
			$hs = $hack;
			$hs =~ s/\[/\\[/g;
			$hs =~ s/\]/\\]/g;
			$dev =~ s/$hs/HACK/;
			$hack =~ s/[{}]//g;
		}
		if ($dev =~ /\((.*)\)/) {
			($tpl, @args) = split(/\s+/, $1);
			@args = grep(s/HACK/$hack/ || 1, @args);
			$dev = &replaceClass($dev, $1, $tpl, @args);
		}
		if ($dev =~ /\w+:(\w+)/) {
			$sub = $1;
			$dev =~ s/:$sub/:[$class{$sub}]/;
		}
		$isSet{$class} = 1 if $set ne '';
		if ($isSet{$class}) {
#			print "$class: [$dev]\n";
		} else {
#			print "$class: $dev\n";
		}
		$class{$class} = $dev;
		if (!$isSet{$class} && $class !~ /^_/) {
			++$classCount;
			$mydev = $dev;
			$mydev =~ s/:\[.*\]/=STRUCT/g;
			@args = split(/\s+/, $mydev);
			my $arity = 1 + $#args;
			my $bits = sprintf("%02x%04x%02x", $atomType{'object'}, $classCount, $arity);
			$definition{$class} = $bits;
#			print "$class r-opcode=$classCount arity=$arity mydev=$mydev\n";
		}
	} elsif (/!op \((.*)\):(.*)/) {
		my ($def, $return_type) = ($1, $2);
		$return_type =~ s/\s+$//;
		my ($op, @args) = split(/ /, $def);
		warn "undefined return type '$return_type'" unless (defined $operatorReturnType{$return_type});
		my $typearity = ($operatorReturnType{$return_type} << 3) + 1 + $#args;
		$definition{$op} = sprintf("%02x%04x%02x", 0xC4, ++$operatorCount, $typearity);
		#print "def $op = $definition{$op}\n";
		my $upper_op = $op;
		$upper_op =~ s/\|/NOT_/;
		$upper_op =~ s/_//;
		$upper_op =~ tr/a-z/A-Z/;
		print OPCODES_H_TMP "#define OPCODE_$upper_op $operatorCount\n";
	} else {
		warn "$_ unparsed\n";
	}
}

foreach $class (sort keys %class) {
	$dclass = $class;
	next if $isSet{$class} || $class =~ /^_/;
#	next if $class{$class} =~ /:/;
#	$dclass =~ s/^mk\.//;
	if (!defined $definition{$dclass}) {
		warn "$dclass ($class{$class}) undefined\n";
	} else {
#		print "$class $definition{$dclass} $class{$class}\n";
	}
}

@lines = <>;
chomp @lines;
$lineNumber = 0;
foreach $i (0..$#lines) {
	$lines[$i] =~ s/;.*//;
	if ($lines[$i] =~ /^([\w\.]+):/) {
		warn "duplicate label '$1'" if defined $label{$1};
		$label{$1} = $i;
		$lines[$i] =~ s/$1://;
	}
	$lines[$i] =~ s/\s+//g;
	$code[$i] = $lineNumber;
	if ($lines[$i] ne '') {
		++$lineNumber;
	}
}

foreach $i (0..$#lines) {
	if ($lines[$i] =~ /[iv]ptr\((.*)\)/) {
		warn "undefined label '$1'" unless defined $label{$1};
		$n = $label{$1};
		$nn = $code[$n];
		$lines[$i] =~ s/\($1\)/($nn)/;
	}
}

foreach $i (0..$#lines) {
	next if $lines[$i] eq '';
	my $bits = '';
	if ($lines[$i] =~ /([iv]ptr)\((.*)\)/) {
		($ptype, $ploc) = ($1, $2);
		warn "undefined ptr type '$ptype'" unless defined $atomType{$ptype};
		$bits = sprintf("%02x%06x", $atomType{$ptype}, $ploc);
	} elsif ($lines[$i] =~ /set\((.*)\)/) {
		$bits = sprintf("%02x%06x", 0xC1, $1);
	} elsif ($lines[$i] =~ /cptr\((.*)\)/) {
		$bits = sprintf("%02x%06x", $atomType{'cptr'}, $1);
	} elsif ($lines[$i] =~ /float\((.*)\)/) {
		$f = pack("f", $1);
		($n) = unpack("I", $f);
		$bits = sprintf("%08x", $n >> 1);
	} elsif ($lines[$i] =~ /boolean\((.*)\)/) {
		$n = ($1 eq 'yes') ? 1 : 0;
		$bits = sprintf("%02x%06x", 0x81, $n);
	} elsif ($lines[$i] =~ /bits\((.*)\)/) {
		$bits = $1;
	} elsif (defined $definition{$lines[$i]}) {
		$bits = $definition{$lines[$i]};
	} elsif (defined $atomType{$lines[$i]}) {
		$bits = sprintf("%02x0000%02x", $atomType{$lines[$i]}, $lines[$i] eq 'timestamp' ? 2 : 0);
	}
	print "$code[$i]: [$bits] $lines[$i]\n";
}

system("cmp opcodes.h.tmp opcodes.h > /dev/null");
if ($? != 0) {
	system("cp opcodes.h.tmp opcodes.h");
}
system("rm opcodes.h.tmp");
