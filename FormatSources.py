from os import system, listdir
from os.path import join as JoinPath, isdir, splitext

def ListFiles(path: str, out_list: list):
	for item in listdir(path):
		fullPath = JoinPath(path, item)
		if isdir(fullPath):
			ListFiles(fullPath, out_list)
			continue

		if splitext(item)[1] in [".cpp", ".c", ".h", ".hpp", ".inl"]:
			out_list.append(fullPath)

files = []
ListFiles("source", files)
ListFiles("tests", files)
ListFiles("example", files)

system("clang-format --style=file -i %s" % (' '.join(files)))