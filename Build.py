from os import system

def Build(presetName: str):
	cmd = "cmake --build --preset %s" % presetName
	system(cmd)

if __name__ == '__main__':
	Build("msvc_debug")
	Build("ninja_debug")