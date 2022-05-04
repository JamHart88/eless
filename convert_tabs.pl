open(FH, 'tabz.txt');

my @new;

foreach my $line (<FH>) {
    $line =~ s/\t/    /g; # Add your spaces here!
    push(@new, $line);
}

close(FH);

open(FH, '>new.txt');
printf(FH $_) foreach (@new);
close(FH);
