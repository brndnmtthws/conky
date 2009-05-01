#!/usr/bin/python

#
# TODO: finish this to update nano/vim syntax files, and also handle config
# settings.
#

import os.path
import re

file_names = dict()
file_names["text_objects"]    = "src/text_object.h"
file_names["conky"]           = "src/conky.c"
file_names["vim_syntax"]      = "extras/vim/syntax/conkyrc.vim"
file_names["nano_syntax"]     = "extras/nano/conky.nanorc"
file_names["variables"]       = "doc/variables.xml"
file_names["config_settings"] = "doc/config_settings.xml"

for fn in file_names.values():
	if not os.path.exists(fn) or not os.path.isfile(fn):
		print "'%s' doesn't exist, or isn't a file" % (fn)
		exit(0)

objects = []

file = open(file_names["text_objects"], "r")
exp = re.compile("\s*OBJ_(\w*).*")
while file:
	line = file.readline()
	if len(line) == 0:
		break
	res = exp.match(line)
	if res:
		obj = res.group(1)
		if not re.match("color\d", obj) and obj != "text":
			# ignore colourN stuff
			objects.append(res.group(1))

doc_objects = []
exp = re.compile("\s*<command><option>(\w*)</option></command>.*")
file = open(file_names["variables"], "r")
while file:
	line = file.readline()
	if len(line) == 0:
		break
	res = exp.match(line)
	if res:
		doc_objects.append(res.group(1))
		if doc_objects[len(doc_objects) - 1] not in objects:
			print "'%s' is documented, but doesn't seem to be an object" % (doc_objects[len(doc_objects) - 1])

for obj in objects:
	if obj not in doc_objects:
		print "'%s' seems to be undocumented" % (obj)
