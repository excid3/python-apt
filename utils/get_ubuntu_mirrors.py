#!/usr/bin/env python
#
#  get_ubuntu_mirrors.py
#
#  Download the latest list with available mirrors from the Ubuntu
#  wiki and extract the hosts from the raw page
#
#  Copyright (c) 2006 Free Software Foundation Europe
#
#  Author: Sebastian Heinlein <glatzor@ubuntu.com>
# 
#  This program is free software; you can redistribute it and/or 
#  modify it under the terms of the GNU General Public License as 
#  published by the Free Software Foundation; either version 2 of the
#  License, or (at your option) any later version.
# 
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
#  USA

import urllib2
import re
import os
import commands
import sys

# the list of official Ubuntu servers
mirrors = []
# path to the local mirror list
list_path = "../data/templates/Ubuntu.mirrors"

req = urllib2.Request("https://wiki.ubuntu.com/Archive?action=raw")

try:
    print "Downloading mirrors list from the Ubuntu wiki..."
    uri=urllib2.urlopen(req)
    p = re.compile('^.*((http|ftp):\/\/[A-Za-z0-9-.:\/_]+).*\n*$')
    for line in uri.readlines():
         if r"[[Anchor(dvd-images)]]" in line:
             break
         if "http://" in line or "ftp://" in line:
             mirrors.append(p.sub(r"\1", line))
    uri.close()
except:
    print "Failed to download or extract the mirrors list!"
    sys.exit(1)

print "Writing local mirrors list: %s" % list_path
list = open(list_path, "w")
for mirror in mirrors:
    list.write("%s\n" % mirror)
list.close()
print "Done."


