#!/usr/bin/python3

# Script to process trace debug output
import string, os, sys, stat, subprocess

TRACEFILE="trace.out"
TEXT_TRACEFILE="trace.txt"

# Create a TRACE file if none exists
def init_trace():
    try:
        os.stat(TEXT_TRACEFILE)
    except OSError:
        os.system("rm -f {}".format(TEXT_TRACEFILE))
        os.mkfifo(TEXT_TRACEFILE)


def hex_conv(s):
    return int(s, 16)

# Load symbols
def load_symbols(prog):
    f = os.popen("nm -o "+prog)
    sym = {}
    for line in f.readlines():
        #print(f"line= {line}")
        fields = line.split()
        #print(f"{fields}")
        v = fields[0].split(':')[1]
        #print ("DBG: fields is {} v is {}".format(fields, v))
        if len(v) > 0 and v[0] == "0":
            val = hex_conv('0x' + v)
            #val = hex_conv(s[0])
            name = subprocess.check_output(["c++filt", fields[2]])
            #print (f"{fields} -> {val} -> {name}")
            sym[val]=name.decode("UTF-8")[:-1]
        
    f.close()
    return sym

# Print a program's trace
def parse_trace(prog):

    sym = load_symbols(prog)

    tr = open(TRACEFILE)
    level = 0
    prevname = ""
    count=0
    done = False
    while not done:
        l = tr.readline()
        if l:
            s = l.split()
            if len(s) == 0:
                continue
            if not (s[0] == "e" or s[0] == "x") :
                print("           {}| {}".format(" "*level, l[:-1]))
            else:
                
                s1 = hex_conv(s[8])
                time = int(s[6])

                if s1 in sym.keys():
                    name = sym[s1]
                else:
                    name = "??"

                if s[0]=="e":
                    level += 1
                    print("{}{}\\ {}".format(time, " "*level, name))
                elif s[0]=="x" :
                    
                    print("{}{}/ {}".format(time, " "*level, name))
                    level -=1
                

        else:
            done = True
    tr.close()


# Main
if __name__=="__main__":
    if len(sys.argv)<2:
        print ("Usage: analysis <binary name>")
        sys.exit(1)
    init_trace()
    parse_trace(sys.argv[1])
