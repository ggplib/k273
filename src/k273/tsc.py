# queries tsc capabilities

import sys

# only works for one cpu
done = False
for line in open("/proc/cpuinfo").readlines():
    if "tsc" in line:
        tokens = line.split()
        for t in tokens:
            if t == "tsc":
                print t, "time stampe counter (TSC) - RDTSC available"
                done = True

            elif t == "rdtscp":
                print t, "Read Time-Stamp Counter and Processor ID - instruction available"

            elif t == "constant_tsc":
                print t, "TSC synchronised between cpus/cores"

            elif t == "nonstop_tsc":
                print t, "TSC does not stop in C states (power managmenet?)"

            elif t == "tsc_adjust":
                print t, "TSC adjustment MSR (TSC value of a core could be changed by some software subsystem using the WRMSR instruction)"

            elif "tsc" in t:
                print "WTF", t

            # also tsc_reliable, 

    if done:
        sys.exit()
