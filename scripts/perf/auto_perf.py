#!/usr/bin/python3

# Run editor with perf stat for several execution durations
# and plot statistics as a function of execution duration

import sys, re, json, getopt
import matplotlib.pyplot as plt
import numpy as np

# Return perf standard output as a string for a given execution duration
def get_perf(duration):
	import subprocess
	command = ['perf', 'stat', 'timeout', str(duration) + 's', '../../bin/editor']
	return subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout.decode('utf-8')


# Parse perf standard output for diverse statistics identified in the 'keys' list
# and return an array containing each measurement 
def parse_output(perf_str, keys):
	output = []
	for key in keys:
		m = re.search(r'([\d\W]+)\s+' + key, perf_str)
		value = int(re.sub('[^0-9]','', m.group(1)))
		output.append(value)

	return output


# Export statistics to a json file
def export_json(stats):
	with open('stats.json', 'w') as data_file:
		json.dump(stats, data_file)


# Import statistics from a json file
def import_json():
	with open('stats.json') as data_file:
		return json.load(data_file)


# Run perfs 'samples' times for each duration in 'durations' (seconds)
# and save collected statistics in a json file
def bench(samples, durations):
	keys = ['context-switches', 'cpu-migrations', 'page-faults', 'cycles', 'instructions', 'branches', 'branch-misses']
	stats = {key:[] for key in keys}

	for duration in durations:
		result = [0 for kk in range(len(keys))]
		result = np.array(result)
		for sample in range(samples):
			print("Duration: " + str(duration) + " Sample #" + str(sample))
			output = get_perf(duration)
			result = result + parse_output(output, keys)
		result = result / samples # Compute mean

		for ii, key in enumerate(keys):
			stats[key].append(result[ii])

	stats['duration'] = durations

	export_json(stats)


# Plot statistics in the 'stats' dictionnary
def display_stats(stats):
	# Additional stats
	num_points = len(stats['duration'])

	stats['branch-misses-percent'] = []
	stats['instructions-per-cycle'] = []
	for ii in range(num_points):
		stats['branch-misses-percent'].append(100 * stats['branch-misses'][ii] / stats['branches'][ii])
		stats['instructions-per-cycle'].append(stats['instructions'][ii] / stats['cycles'][ii])

	print(stats)

	fig, axs = plt.subplots(3,3)
	fig.tight_layout()
	keys = list(stats.keys())
	keys.remove('duration')
	for ii, kk in enumerate(keys):
		xx = ii//3
		yy = ii%3
		if kk != 'duration':
			axs[xx,yy].plot(stats['duration'], stats[kk], linestyle='-', marker='o', color='orange', label=kk)
			axs[xx,yy].set_xlabel('duration')
			axs[xx,yy].set_ylabel(kk)

	plt.show()


def main(argv):
	try:
		opts, args = getopt.getopt(argv, "hb", ["help"])
	except getopt.GetoptError as err:
		print(str(err))
		sys.exit(2)

	mode = 'p'
	for o, a in opts:
		if o in ("-b", "--bench"):
			mode = 'b'
		else:
			assert False, "unhandled option"

	if mode == 'p':
		stats = import_json()
		display_stats(stats)
	elif mode == 'b':
		bench(3, [2, 3, 4, 5, 10, 15, 30, 60, 120])
		stats = import_json()
		display_stats(stats)


if __name__ == '__main__':
    main(sys.argv[1:])