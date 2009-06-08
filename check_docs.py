#!/usr/bin/python
#
# This script will check the documentation consistency against the code.  It
# doesn't check the actual accuracy of the documentation, it just ensures that
# everything is documented and that nothing which doesn't exist in Conky
# appears in the documentation.
#
# This script also updates the vim and nano syntax files so it doesn't have to
# be done manually.
#
# Requires the ElementTree Python module for the sorting stuff, see:
# http://effbot.org/zone/element-index.htm
#
# You should also install htmltidy, but it's not necessary.
#

import os.path
import re
import sys

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
		exit(1)

print 'sorting/tidying docs...'

# sort the docs by variable/config setting
import string
import xml.etree.ElementTree as ET

vars_xml = ET.parse(file_names['variables'])
config_xml = ET.parse(file_names['config_settings'])

getkey = lambda x: x.findtext('term/command/option')

vars = vars_xml.getroot()
vars[:] = sorted(vars, key=getkey)

configs = config_xml.getroot()
configs[:] = sorted(configs, key=getkey)

vars_xml.write(file_names['variables'])
config_xml.write(file_names['config_settings'])

def tidy(file):
	command = ['tidy', '-qim', '-xml', '-utf8', '--indent-spaces', '4']
	os.system('%s %s 2>/dev/null' % (string.join(command), file))

tidy(file_names['variables'])
tidy(file_names['config_settings'])

#
# Do all the objects first
#

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
file.close()

doc_objects = []
exp = re.compile("\s*<command><option>(\w*)</option></command>.*")
print "checking docs -> objs consistency (in %s)" % (file_names["text_objects"])
for var in vars:
	term = getkey(var)
	doc_objects.append(term)
	if ['templaten', 'colorn'].count(doc_objects[len(doc_objects) - 1].lower()):
		# ignore these
		continue
	if doc_objects[len(doc_objects) - 1] not in objects:
		print "   '%s' is documented, but doesn't seem to be an object" % (doc_objects[len(doc_objects) - 1])
print "done\n"

print "checking objs -> docs consistency (in %s)" % (file_names["variables"])
for obj in objects:
	if obj not in doc_objects:
		print "   '%s' seems to be undocumented" % (obj)
print "done\n"

#
# Now we'll do config settings
#

config_entries = []

file = open(file_names["conky"], "r")
exp1 = re.compile('\s*CONF\("(\w*)".*')
exp2 = re.compile('\s*CONF2\("(\w*)".*')
exp3 = re.compile('\s*CONF3\("(\w*)".*')
while file:
	line = file.readline()
	if len(line) == 0:
		break
	res = exp1.match(line)
	if not res:
		res = exp2.match(line)
	if not res:
		res = exp3.match(line)
	if res:
		conf = res.group(1)
		if re.match("color\d", conf):
			conf = "colorN"
		if config_entries.count(conf) == 0:
			config_entries.append(conf)
file.close()

doc_configs = []
print "checking docs -> configs consistency (in %s)" % (file_names["conky"])
for config in configs:
	term = getkey(config)
	doc_configs.append(term)
	if ['text', 'templaten'].count(doc_configs[len(doc_configs) - 1].lower()):
		# ignore these
		continue
	if doc_configs[len(doc_configs) - 1] not in config_entries:
		print "   '%s' is documented, but doesn't seem to be a config setting" % (doc_configs[len(doc_configs) - 1])
print "done\n"

print "checking configs -> docs consistency (in %s)" % (file_names["config_settings"])
for obj in config_entries:
	if obj != "text" and obj != "template" and obj not in doc_configs:
		print "   '%s' seems to be undocumented" % (obj)
print "done\n"



# Cheat and add the colour/template stuff.

for i in range(0, 10):
	objects.append("color" + str(i))
	config_entries.append("color" + str(i))
	objects.append("template" + str(i))
	config_entries.append("template" + str(i))

# Finally, sort everything.
objects.sort()
config_entries.sort()

#
# Update nano syntax stuff
#

print "updating nano syntax...",
sys.stdout.flush()
file = open(file_names["nano_syntax"], "rw+")
lines = []
while file:
	line = file.readline()
	if len(line) == 0:
		break
	lines.append(line)

# find the line we want to update
for line in lines:
	if re.match("color green ", line):
		idx = lines.index(line)
		lines.pop(idx) # remove old line
		line = 'color green "\<('
		for obj in config_entries:
			line += "%s|" % (obj)
		line = line[:len(line) - 1]
		line += ')\>"\n'
		lines.insert(idx, line)
	if re.match("color brightblue ", line):
		idx = lines.index(line)
		lines.pop(idx) # remove old line
		line = 'color brightblue "\<('
		for obj in objects:
			line += "%s|" % (obj)
		line = line[:len(line) - 1]
		line += ')\>"\n'
		lines.insert(idx, line)
		break # want to ignore everything after this line
file.truncate(0)
file.seek(0)
file.writelines(lines)
file.close()
print "done."

#
# Update vim syntax stuff
#

print "updating vim syntax...",
sys.stdout.flush()
file = open(file_names["vim_syntax"], "rw+")
lines = []
while file:
	line = file.readline()
	if len(line) == 0:
		break
	lines.append(line)

# find the line we want to update
for line in lines:
	if re.match("syn keyword ConkyrcSetting ", line):
		idx = lines.index(line)
		lines.pop(idx) # remove old line
		line = 'syn keyword ConkyrcSetting '
		for obj in config_entries:
			line += "%s " % (obj)
		line = line[:len(line) - 1]
		line += '\n'
		lines.insert(idx, line)
	if re.match("syn keyword ConkyrcVarName contained nextgroup=ConkyrcNumber,ConkyrcColour skipwhite ", line):
		idx = lines.index(line)
		lines.pop(idx) # remove old line
		line = 'syn keyword ConkyrcVarName contained nextgroup=ConkyrcNumber,ConkyrcColour skipwhite '
		for obj in objects:
			line += "%s " % (obj)
		line = line[:len(line) - 1]
		line += '\n'
		lines.insert(idx, line)
		break # want to ignore everything after this line
file.truncate(0)
file.seek(0)
file.writelines(lines)
file.close()

print "done."
