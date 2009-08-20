#!/usr/bin/env python
#
#  get_ubuntu_lp_mirrors.py
#
#  Download the latest list with available Ubuntu mirrors from Launchpad.net
#  and extract the hosts from the raw page
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
import sys

# the list of official Ubuntu servers
mirrors = []
# path to the local mirror list
list_path = "../data/templates/Ubuntu.mirrors"


try:
    f = open("/usr/share/iso-codes/iso_3166.tab", "r")
    lines = f.readlines()
    f.close()
except:
    print "Could not read country information"
    sys.exit(1)

countries = {}
for line in lines:
    parts = line.split("\t")
    countries[parts[1].strip()] = parts[0].lower()

req = urllib2.Request("https://launchpad.net/ubuntu/+archivemirrors")
print "Downloading mirrors list from Launchpad..."
try:
    uri=urllib2.urlopen(req)
    content = uri.read()
    uri.close()
except:
    print "Failed to download or extract the mirrors list!"
    sys.exit(1)

content = content.replace("\n", "")

content_splits = re.split(r'<tr class="highlighted"',
                          re.findall(r'<table class="listing" '
                                      'id="mirrors_list">.+?</table>',
                                     content)[0])
lines=[]
def find(split):
    country = re.search(r"<strong>(.+?)</strong>", split)
    if not country:
        return
    if countries.has_key(country.group(1)):
        lines.append("#LOC:%s" % countries[country.group(1)].upper())
    else:
        lines.append("#LOC:%s" % country.group(1))
    # FIXME: currently the protocols are hardcoded: ftp http
    urls = re.findall(r'<a href="(?![a-zA-Z:/_\-]+launchpad.+?">)'
                       '(((http)|(ftp)).+?)">',
                      split)
    map(lambda u: lines.append(u[0]), urls)

map(find, content_splits)

print "Writing local mirrors list: %s" % list_path
list = open(list_path, "w")
for line in lines:
    list.write("%s\n" % line)
list.close()
print "Done."
