# This script is for packaging up all my *compiled*
#     projects into one folder, so that I can put
#     all the binaries into a Github Release

from importlib import import_module
from shutil import copy, rmtree
from os import path, makedirs

OUT = "out"

def mkdir(name: str):
	if not path.isdir(name):
		makedirs(name)

class Interface:
	def __init__(self, name: str):
		self.name = name
		self.out = path.join(OUT, name)
		self.is_standalone = False

	def file(self, file: str):
		src = path.join(self.name, file)
		copy(src, self.out)
	#	print(f"{src} file added")

	def standalone(self, file: str):
		if self.is_standalone:
			raise Exception("should be standalone, but added more than one")
		src = path.join(self.name, file)
		copy(src, OUT)
		self.is_standalone = True

def package(name: str):

	# import the function we will use to "package" a compiled project
	module = import_module(name + ".package")

	# Setup output directory
	mkdir(path.join(OUT, name))
	
	# Call "package" to add the binary to the output directory
	module.package(Interface(name))


def cleanup(name: str):
	rmtree(path.join(OUT, name))


def main():
	projects = [
		"Electric_Field"
	]

	for project in projects:
		print(f"Project {project:32}", end="")
		try:
			package(project)
			print(" added")
		except:
			cleanup(project)
			print("")


if __name__ == "__main__":
	main()
