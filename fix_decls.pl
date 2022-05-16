#!/usr/bin/perl -w
use strict;

# Turn this:

# public void
# opt_shift(type, s)
# 	int type;
# 	char *s;
# {

# in to this:

# public void opt_shift(int type, char *s)
# {

# Could be "public" or "static"

my $fname = "";


sub main {
    if ($#ARGV == 0) {
        $fname = $ARGV[0];
    } else {
        print("Error - need to provide filename");
        exit;
    }

    open (DATA, $fname);

    my $buff = "";
    my $found_func_defn = 0;

    while (<DATA>) {

        my $line = $_;

        if ($line =~ /^{/) {
            $found_func_defn = 0;
            

            my @lines = split("\n", $buff);
            my $count = 0;
            my $num_lines = $#lines + 1;
            
            print ("// -------------------------------------------\n");
            print ("// Converted from C to C++ - C below\n");
            foreach my $l (@lines) {
                print ("// $l\n");
            }
            
            
            foreach my $l  (@lines) {
                
                #print (">>> $l\n");
                $count++;
                if ($count == 1) {
                    print ("$l ");
                }
                
                if ($count == 2) {
                    if ($l =~ /\(void\)/) {
                        print ("$l\n");
                    } elsif ($line =~ /\(\)/) {
                        print ("$l\n");
                    } else {
                        if ($l =~ /^(.*?)\(/) {
                            print ("$1(");
                        } else {
                            print ("ERROR - not standard func decl -> $buff\n");
                            exit;
                        }

                    }
                } 

                if ($count > 2) {
                    $l =~ s/\t//g;
                    
                    if ($count == $num_lines) {
                        $l =~ s/;/\)\n/;
                    } else {
                        $l =~ s/;/, /;
                    }
                    print ("$l");
                }
            }

        }

        if ($found_func_defn == 1) {
            if ($line =~ /^extern/) {
                $found_func_defn = 0;
                 
            } else {
                $buff = $buff . $line;
            }
        }

        if ($line =~ /^(public|static) (.*)$/) {
            if ($line =~ /;$/) {

            } else {
                $buff = $line;
                $found_func_defn = 1;
            }
        }

        if ($found_func_defn == 0) {

            if ($line =~ /error\(/) {
                $line =~ s/error\(/error\(\(char *\)/;
            }
            $line =~ s/\t/    /;
            print ("$line");
        }
        
    }
}




main;
