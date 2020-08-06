import os
import datetime

fname = str(datetime.datetime.now().strftime("%I%M%S%d%m%Y"))

logname = fname + ".txt"
valgrind = "valgrind --tool=callgrind --callgrind-out-file={} ./build/apps/chess20".format(logname)

os.system(valgrind)

imgname = fname + ".svg"

gprof2dot = "gprof2dot -f callgrind {} | dot -Tsvg -o {}".format(logname, imgname)

os.system(gprof2dot)
