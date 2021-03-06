#!/usr/bin/env python

import sys
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

nthreads_axis = []
all_results = []

for filename in sys.argv[1:]:

	fp = open(filename, "r")

	local_nthreads_axis = []
	local_results = []

	line = fp.readline()
	while line:
		if "num_threads:" in line:
			nthreads = int(line.split()[1])
			local_nthreads_axis.append(nthreads)
		elif "Ops/usec:" in line:
			throughput = float(line.split()[1])
			local_results.append(throughput)

		line = fp.readline()

	fp.close()

	if len(nthreads_axis) == 0:
		nthreads_axis = list(local_nthreads_axis)
	elif len(nthreads_axis) != len(local_nthreads_axis):
		print "File", filename, "contains less nthreads values."
		sys.exit(1)
	elif nthreads_axis != local_nthreads_axis:
		print "File", filename, "contains different nthreads values."
		sys.exit(1)

	all_results.append(local_results)

print nthreads_axis
print all_results

ax = plt.subplot("111")

for res in all_results:
	ax.plot(np.arange(len(nthreads_axis)), res)

#plt.title()
plt.grid(True)
plt.xticks(np.arange(len(nthreads_axis)), nthreads_axis)
ax.set_xlabel("#threads")
ax.set_ylabel("throuhput (ops/usec)")
plt.savefig("nthreads-VS-throughput.pdf")
