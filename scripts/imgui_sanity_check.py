#!/usr/bin/python3
import sys, os, re
from common import bcolors
from collections import Counter
from glob import glob

def list_imgui_sources(walk_dirs):
	# Walk input directories recursively
	list_files = []
	for walk_dir in walk_dirs:
		walk_dir_absolute = os.path.abspath(walk_dir)

		for ext in ["*.cpp","*.h","*.hpp"]:
			list_files.extend([y for x in os.walk(walk_dir_absolute) for y in glob(os.path.join(x[0], ext))])

	# Only retain files that contain "ImGui::"
	imgui_sources = []
	for file in list_files:
		with open(file) as f:
			if "ImGui::" in f.read():
				imgui_sources.append(file)

	return imgui_sources



g_be = {"Begin":"End", "BeginMenuBar":"EndMenuBar", "BeginMainMenuBar":"EndMainMenuBar",
		"BeginMenu":"EndMenu", "BeginTooltip":"EndTooltip", "BeginPopup":"EndPopup",
		"BeginPopupContextItem":"EndPopup", "BeginPopupModal":"EndPopup", "BeginPopupContextWindow":"EndPopup",
		"BeginPopupContextVoid":"EndPopup", "BeginChild":"EndChild", "BeginCombo":"EndCombo",
		"BeginDragDropSource": "EndDragDropSource", "BeginDragDropTarget":"EndDragDropTarget",
		"BeginChildFrame":"EndChildFrame", "BeginTabBar":"EndTabBar", "BeginTabItem":"EndTabItem"}

def check_begin_end_mismatch(file):
	global g_be

	source = ""
	with open(file) as f:
		source = f.read()

	# Extract all ImGui::Begin...() and ImGui::End...() calls
	begins = re.findall(r"ImGui::(Begin.*?)\(.*?\)", source)
	ends   = re.findall(r"ImGui::(End.*?)\(.*?\)", source)
	expected_ends = [g_be[b] for b in begins]

	ecount  = Counter(ends)
	eecount = Counter(expected_ends)

	missing_ends = dict(eecount - ecount)
	if missing_ends:
		for key, value in missing_ends.items():
			print(bcolors.RED + "Possible miss: " + str(value) + " ImGui::" + key + "()" + bcolors.ENDC)
		return False

	return True


def main(argv):
	print("ImGui sanity check")
	walk_dirs = ["../source/Applications/Editor", "../source/Erwin"]

	imgui_sources = list_imgui_sources(walk_dirs)

	no_problem = True
	for source_file in imgui_sources:
		source_ok = True
		source_ok &= check_begin_end_mismatch(source_file)
		# Other checks here...
		if not source_ok:
			print("In: " + source_file)
			print("--------------------------------")
		no_problem &= source_ok

	if no_problem:
		print(bcolors.GREEN + "You are a god." + bcolors.ENDC)


if __name__ == '__main__':
	main(sys.argv[1:])