#!/usr/bin/env python
#  
#  distinfo.py - provide meta information for distro repositories
#
#  Copyright (c) 2005 Gustavo Noronha Silva
#                2006-2007 Sebastian Heinlein
#
#  Author: Gustavo Noronha Silva <kov@debian.org>
#          Sebastian Heinlein <glatzor@ubuntu.com>
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

import os
import gettext
from os import getenv
import ConfigParser
import string
import apt_pkg

#from gettext import gettext as _
import gettext
def _(s): return gettext.dgettext("python-apt", s)

import re

class Template:
    def __init__(self):
        self.name = None
        self.child = False
        self.parents = []          # ref to parent template(s)
        self.match_name = None
        self.description = None
        self.base_uri = None
        self.type = None
        self.components = []
        self.children = []
        self.match_uri = None
        self.mirror_set = {}
        self.distribution = None
        self.available = True

    def has_component(self, comp):
        ''' Check if the distribution provides the given component '''
        return comp in map(lambda c: c.name, self.components)
    
    def is_mirror(self, url):
        ''' Check if a given url of a repository is a valid mirror '''
        proto, hostname, dir = split_url(url)
        if self.mirror_set.has_key(hostname):
            return self.mirror_set[hostname].has_repository(proto, dir)
        else:
            return False

class Component:
    def __init__(self, name, desc=None, long_desc=None):
        self.name = name
        self.description = desc
        self.description_long = long_desc
    def get_description(self):
        if self.description_long != None:
            return self.description_long
        elif self.description != None:
            return self.description
        else:
            return None
    def set_description(self, desc):
        self.description = desc
    def set_description_long(self, desc):
        self.description_long = desc
    def get_description_long(self):
        return self.description_long

class Mirror:
    ''' Storage for mirror related information '''
    def __init__(self, proto, hostname, dir, location=None):
        self.hostname = hostname
        self.repositories = []
        self.add_repository(proto, dir)
        self.location = location
    def add_repository(self, proto, dir):
        self.repositories.append(Repository(proto, dir))
    def get_repositories_for_proto(self, proto):
        return filter(lambda r: r.proto == proto, self.repositories)
    def has_repository(self, proto, dir):
        if dir is None:
            return False
        for r in self.repositories:
            if r.proto == proto and dir in r.dir:
                return True
        return False
    def get_repo_urls(self):
        return map(lambda r: r.get_url(self.hostname), self.repositories)
    def get_location(self):
        return self.location
    def set_location(self, location):
        self.location = location

class Repository:
    def __init__(self, proto, dir):
        self.proto = proto
        self.dir = dir
    def get_info(self):
        return self.proto, self.dir
    def get_url(self, hostname):
        return "%s://%s/%s" % (self.proto, hostname, self.dir)

def split_url(url):
    ''' split a given URL into the protocoll, the hostname and the dir part '''
    return map(lambda a,b: a, re.split(":*\/+", url, maxsplit=2),
                              [None, None, None])

class DistInfo:
    def __init__(self,
                 dist = None,
                 base_dir = "/usr/share/python-apt/templates"):
        self.metarelease_uri = ''
        self.templates = []
        self.arch = apt_pkg.Config.Find("APT::Architecture")

        location = None
        match_loc = re.compile(r"^#LOC:(.+)$")
        match_mirror_line = re.compile(r"^(#LOC:.+)|(((http)|(ftp)|(rsync)|(file)|(https))://[A-Za-z/\.:\-_]+)$")
        #match_mirror_line = re.compile(r".+")

        if not dist:
            pipe = os.popen("lsb_release -i -s")
            dist = pipe.read().strip()
            pipe.close()
            del pipe

        self.dist = dist


        map_mirror_sets = {}

        dist_fname = "%s/%s.info" % (base_dir, dist)
        dist_file = open (dist_fname)
        if not dist_file:
            return
        template = None
        component = None
        for line in dist_file:
            tokens = line.split (':', 1)
            if len (tokens) < 2:
                continue
            field = tokens[0].strip ()
            value = tokens[1].strip ()
            if field == 'ChangelogURI':
                self.changelogs_uri = _(value)
            elif field == 'MetaReleaseURI':
                self.metarelease_uri = value
            elif field == 'Suite':
                self.finish_template(template, component)
                component=None
                template = Template()
                template.name = value
                template.distribution = dist
                template.match_name = "^%s$" % value
            elif field == 'MatchName':
                template.match_name = value
            elif field == 'ParentSuite':
                template.child = True
                for nanny in self.templates:
                    # look for parent and add back ref to it
                    if nanny.name == value:
                        template.parents.append(nanny)
                        nanny.children.append(template)
            elif field == 'Available':
                template.available = value
            elif field == 'RepositoryType':
                template.type = value
            elif field == 'BaseURI' and not template.base_uri:
                template.base_uri = value
            elif field == 'BaseURI-%s' % self.arch:
                template.base_uri = value
            elif field == 'MatchURI' and not template.match_uri:
                template.match_uri = value
            elif field == 'MatchURI-%s' % self.arch:
                template.match_uri = value
            elif (field == 'MirrorsFile' or 
                  field == 'MirrorsFile-%s' % self.arch):
                if not map_mirror_sets.has_key(value):
                    mirror_set = {}
                    try:
                        mirror_data = filter(match_mirror_line.match,
                                             map(string.strip, open(value)))
                    except:
                        print "WARNING: Failed to read mirror file"
                        mirror_data = []
                    for line in mirror_data:
                        if line.startswith("#LOC:"):
                            location = match_loc.sub(r"\1", line)
                            continue
                        (proto, hostname, dir) = split_url(line)
                        if mirror_set.has_key(hostname):
                            mirror_set[hostname].add_repository(proto, dir)
                        else:
                            mirror_set[hostname] = Mirror(proto, hostname, dir, location)
                    map_mirror_sets[value] = mirror_set
                template.mirror_set = map_mirror_sets[value]
            elif field == 'Description':
                template.description = _(value)
            elif field == 'Component':
                if component and not template.has_component(component.name):
                    template.components.append(component)
                component = Component(value)
            elif field == 'CompDescription':
                component.set_description(_(value))
            elif field == 'CompDescriptionLong':
                component.set_description_long(_(value))
        self.finish_template(template, component)
        template=None
        component=None

    def finish_template(self, template, component):
        " finish the current tempalte "
        if not template:
            return
        # reuse some properties of the parent template
        if template.match_uri == None and template.child:
            for t in template.parents:
                if t.match_uri:
                    template.match_uri = t.match_uri
                    break
        if template.mirror_set == {} and template.child:
            for t in template.parents:
                if t.match_uri:
                    template.mirror_set = t.mirror_set
                    break
        if component and not template.has_component(component.name):
            template.components.append(component)
            component = None
        self.templates.append(template)    


if __name__ == "__main__":
    d = DistInfo ("Ubuntu", "/usr/share/python-apt/templates")
    print d.changelogs_uri
    for template in d.templates:
        print "\nSuite: %s" % template.name
        print "Desc: %s" % template.description
        print "BaseURI: %s" % template.base_uri
        print "MatchURI: %s" % template.match_uri
        if template.mirror_set != {}:
            print "Mirrors: %s" % template.mirror_set.keys()
        for comp in template.components:
            print " %s -%s -%s" % (comp.name, 
                                   comp.description, 
                                   comp.description_long)
        for child in template.children:
            print "  %s" % child.description
