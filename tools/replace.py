#!/usr/bin/env

import os.path
import glob
import argparse

def process_a_file(file_path, old_str, new_str, verbose, test):
	if os.path.isfile(file_path):
		if verbose: print('\tprocessing file: ' + file_path)
	else:
		if verbose: print('\tWARNING: file "' + file_path + '" does not exist.')
		return 0

	if test: return 1
	
	file = open(file_path, 'r+')
	lines = file.readlines()
	new_lines = [l.replace(old_str, new_str) for l in lines]
	file.seek(0)
	file.truncate(0)
	file.write(''.join(new_lines))
	file.close()
	return 1

def process_a_directory(dir_path, search_filter, old_str, new_str, verbose, test, depth=0):
	if not dir_path.endswith(os.sep): dir_path += os.sep
	if os.path.isdir(dir_path):
		if verbose: print('\t' * depth + ' - processing dir: ' + dir_path + search_filter)
	else:
		if verbose: print('WARNING: directory "' + dir_path + '" does not exist')
		return 0
	filepaths = glob.iglob(dir_path + search_filter)
	file_count = 0
	for path in filepaths:
		if (os.path.isfile(path)):
			file_count += process_a_file(path, old_str, new_str, verbose, test)
	dirpaths = glob.glob(dir_path + '**/', recursive=True)
	for subdir in dirpaths:
		if (subdir != dir_path):
			file_count += process_a_directory(subdir, search_filter, old_str, new_str, verbose, test, depth + 1)
	return file_count

def make_list(arg):
	return arg if isinstance(arg, list) else [arg]

def main():
	parser = argparse.ArgumentParser(description='Apply string replacement for all source code files in a codebase')
	parser.add_argument('dirs', type=str, nargs='?', default=[], help='Root directories for the codebase')
	parser.add_argument('--files', '--file', type=str, nargs='+', default=[], help='Indivisuals file to process')
	parser.add_argument('--file-filters', '--file-filter', type=str, nargs="+", default='*', help='File extention filter to select source code.')
	parser.add_argument('--new-str', type=str, help='string to be replaces.')
	parser.add_argument('--old-str', type=str, help='new string to put inplace of old ones.')
	parser.add_argument('-run', '-r', action="store_true")
	parser.add_argument('-verbose', '-v', action="store_true")
	argument = parser.parse_args()

	verb = argument.verbose
	run = argument.run
	test = not run
	file_filters = make_list(argument.file_filters)
	files = make_list(argument.files)
	dirs = make_list(argument.dirs)

	old_str = argument.old_str
	new_str = argument.new_str

	file_count = 0
	for f in files:
		file_count += process_a_file(f, old_str, new_str, verb, test)
	for d in dirs:
		for ffilter in file_filters:
			file_count += process_a_directory(d, ffilter, old_str, new_str, verb, test)

	print("-- File count: %d." %file_count)

	if test:
		print('>>> !!! ATTENTION !!! <<<')
		print('>>> Running in test mode, no file editing is conducted.')
		print('>>> Run with "--inplace" to apply after confirming the test output')
		print('>>> !!! ATTENTION !!! <<<')

if __name__ == "__main__":
	main()