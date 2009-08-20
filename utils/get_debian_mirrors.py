#!/usr/bin/env python
#
#  get_debian_mirrors.py
#
#  Download the latest list with available mirrors from the Debian 
#  website and extract the hosts from the raw page
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
list_path = "../data/templates/Debian.mirrors"

req = urllib2.Request("http://www.debian.org/mirror/mirrors_full")
match = re.compile("^.*>([A-Za-z0-9-.\/_]+)<\/a>.*\n$")
match_location = re.compile('^<strong><a name="([A-Z]+)">.*')

def add_sites(line, proto, sites, mirror_type):
    path = match.sub(r"\1", line)
    for site in sites:
        mirror_type.append("%s://%s%s\n" % (proto, site.lstrip(), path))

try:
    print "Downloading mirrors list from the Debian website..."
    uri=urllib2.urlopen(req)
    for line in uri.readlines():
        if line.startswith('<strong><a name="'):
            location = match_location.sub(r"\1", line)
            if location:
                mirrors.append("#LOC:%s" % location)
        if line.startswith("Site:"):
            sites = line[6:-1].split(",")
        elif line.startswith('Packages over HTTP'):
            add_sites(line, "http", sites, mirrors)
        elif line.startswith('Packages over FTP'):
            add_sites(line, "ftp", sites, mirrors)
    uri.close()
except:
    print "Failed to download or to extract the mirrors list!"
    sys.exit(1)

print "Writing local mirrors list: %s" % list_path
list = open(list_path, "w")
for mirror in mirrors:
    list.write("%s" % mirror)
list.close()
print "Done."
