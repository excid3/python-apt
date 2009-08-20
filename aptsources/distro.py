#  distro.py - Provide a distro abstraction of the sources.list
#
#  Copyright (c) 2004-2007 Canonical Ltd.
#                2006-2007 Sebastian Heinlein
#  
#  Authors: Sebastian Heinlein <glatzor@ubuntu.com>
#           Michael Vogt <mvo@debian.org>
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

import string
import gettext
import re
import os
import sys

import gettext
def _(s): return gettext.dgettext("python-apt", s)


class NoDistroTemplateException(Exception):
  pass

class Distribution:
  def __init__(self, id, codename, description, release):
    """ Container for distribution specific informations """
    # LSB information
    self.id = id
    self.codename = codename
    self.description = description
    self.release = release

    self.binary_type = "deb"
    self.source_type = "deb-src"

  def get_sources(self, sourceslist):
    """
    Find the corresponding template, main and child sources 
    for the distribution 
    """

    self.sourceslist = sourceslist
    # corresponding sources
    self.source_template = None
    self.child_sources = []
    self.main_sources = []
    self.disabled_sources = []
    self.cdrom_sources = []
    self.download_comps = []
    self.enabled_comps = []
    self.cdrom_comps = []
    self.used_media = []
    self.get_source_code = False
    self.source_code_sources = []

    # location of the sources
    self.default_server = ""
    self.main_server = ""
    self.nearest_server = ""
    self.used_servers = []

    # find the distro template
    for template in self.sourceslist.matcher.templates:
        if self.is_codename(template.name) and\
           template.distribution == self.id:
            #print "yeah! found a template for %s" % self.description
            #print template.description, template.base_uri, template.components
            self.source_template = template
            break
    if self.source_template == None:
        raise (NoDistroTemplateException,
               "Error: could not find a distribution template")

    # find main and child sources
    media = []
    comps = []
    cdrom_comps = []
    enabled_comps = []
    source_code = []
    for source in self.sourceslist.list:
        if source.invalid == False and\
           self.is_codename(source.dist) and\
           source.template and\
           self.is_codename(source.template.name):
            #print "yeah! found a distro repo:  %s" % source.line
            # cdroms need do be handled differently
            if source.uri.startswith("cdrom:") and \
               source.disabled == False:
                self.cdrom_sources.append(source)
                cdrom_comps.extend(source.comps)
            elif source.uri.startswith("cdrom:") and \
                 source.disabled == True:
                self.cdrom_sources.append(source)
            elif source.type == self.binary_type  and \
                 source.disabled == False:
                self.main_sources.append(source)
                comps.extend(source.comps)
                media.append(source.uri)
            elif source.type == self.binary_type and \
                 source.disabled == True:
                self.disabled_sources.append(source)
            elif source.type == self.source_type and source.disabled == False:
                self.source_code_sources.append(source)
            elif source.type == self.source_type and source.disabled == True:
                self.disabled_sources.append(source)
        if source.invalid == False and\
           source.template in self.source_template.children:
            if source.disabled == False and source.type == self.binary_type:
                self.child_sources.append(source)
            elif source.disabled == False and source.type == self.source_type:
                self.source_code_sources.append(source)
            else:
                self.disabled_sources.append(source)
    self.download_comps = set(comps)
    self.cdrom_comps = set(cdrom_comps)
    enabled_comps.extend(comps)
    enabled_comps.extend(cdrom_comps)
    self.enabled_comps = set(enabled_comps)
    self.used_media = set(media)

    self.get_mirrors()
  
  def get_mirrors(self, mirror_template=None):
    """
    Provide a set of mirrors where you can get the distribution from
    """
    # the main server is stored in the template
    self.main_server = self.source_template.base_uri

    # other used servers
    for medium in self.used_media:
        if not medium.startswith("cdrom:"):
            # seems to be a network source
            self.used_servers.append(medium)

    if len(self.main_sources) == 0:
        self.default_server = self.main_server
    else:
        self.default_server = self.main_sources[0].uri

    # get a list of country codes and real names
    self.countries = {}
    try:
        f = open("/usr/share/iso-codes/iso_3166.tab", "r")
        lines = f.readlines()
        for line in lines:
            parts = line.split("\t")
            self.countries[parts[0].lower()] = parts[1].strip()
    except:
        print "could not open file '%s'" % file
    else:
        f.close()

    # try to guess the nearest mirror from the locale
    self.country = None
    self.country_code = None
    locale = os.getenv("LANG", default="en_UK")
    a = locale.find("_")
    z = locale.find(".")
    if z == -1:
        z = len(locale)
    country_code = locale[a+1:z].lower()

    if mirror_template:
      self.nearest_server = mirror_template % country_code

    if self.countries.has_key(country_code):
        self.country = self.countries[country_code]
        self.country_code = country_code

  def _get_mirror_name(self, server):
      ''' Try to get a human readable name for the main mirror of a country
          Customize for different distributions '''
      country = None
      i = server.find("://")
      l = server.find(".archive.ubuntu.com")
      if i != -1 and l != -1:
          country = server[i+len("://"):l]
      if self.countries.has_key(country):
          # TRANSLATORS: %s is a country
          return _("Server for %s") % \
                 gettext.dgettext("iso_3166",
                                  self.countries[country].rstrip()).rstrip()
      else:
          return("%s" % server.rstrip("/ "))

  def get_server_list(self):
    ''' Return a list of used and suggested servers '''
    def compare_mirrors(mir1, mir2):
        '''Helper function that handles comaprision of mirror urls
           that could contain trailing slashes'''
        return re.match(mir1.strip("/ "), mir2.rstrip("/ "))
    
    # Store all available servers:
    # Name, URI, active
    mirrors = []
    if len(self.used_servers) < 1 or \
       (len(self.used_servers) == 1 and \
        compare_mirrors(self.used_servers[0], self.main_server)):
        mirrors.append([_("Main server"), self.main_server, True]) 
        mirrors.append([self._get_mirror_name(self.nearest_server), 
                       self.nearest_server, False])
    elif len(self.used_servers) == 1 and not \
         compare_mirrors(self.used_servers[0], self.main_server):
        mirrors.append([_("Main server"), self.main_server, False]) 
        # Only one server is used
        server = self.used_servers[0]

        # Append the nearest server if it's not already used            
        if not compare_mirrors(server, self.nearest_server):
            mirrors.append([self._get_mirror_name(self.nearest_server), 
                           self.nearest_server, False])
        mirrors.append([self._get_mirror_name(server), server, True])

    elif len(self.used_servers) > 1:
        # More than one server is used. Since we don't handle this case
        # in the user interface we set "custom servers" to true and 
        # append a list of all used servers 
        mirrors.append([_("Main server"), self.main_server, False])
        mirrors.append([self._get_mirror_name(self.nearest_server), 
                                        self.nearest_server, False])
        mirrors.append([_("Custom servers"), None, True])
        for server in self.used_servers:
            if compare_mirrors(server, self.nearest_server) or\
               compare_mirrors(server, self.main_server):
                continue
            elif not [self._get_mirror_name(server), server, False] in mirrors:
                mirrors.append([self._get_mirror_name(server), server, False])

    return mirrors

  def add_source(self, type=None, 
                 uri=None, dist=None, comps=None, comment=""):
    """
    Add distribution specific sources
    """
    if uri == None:
        # FIXME: Add support for the server selector
        uri = self.default_server
    if dist == None:
        dist = self.codename
    if comps == None:
        comps = list(self.enabled_comps)
    if type == None:
        type = self.binary_type
    new_source = self.sourceslist.add(type, uri, dist, comps, comment)
    # if source code is enabled add a deb-src line after the new
    # source
    if self.get_source_code == True and type == self.binary_type:
        self.sourceslist.add(self.source_type, uri, dist, comps, comment,
                             file=new_source.file,
                             pos=self.sourceslist.list.index(new_source)+1)

  def enable_component(self, comp):
    """
    Enable a component in all main, child and source code sources
    (excluding cdrom based sources)

    comp:         the component that should be enabled
    """
    def add_component_only_once(source, comps_per_dist):
        """
        Check if we already added the component to the repository, since
        a repository could be splitted into different apt lines. If not
        add the component
        """
        # if we don't that distro, just reutnr (can happen for e.g.
        # dapper-update only in deb-src
        if not comps_per_dist.has_key(source.dist):
          return
        # if we have seen this component already for this distro,
        # return (nothing to do
        if comp in comps_per_dist[source.dist]:
          return
        # add it
        source.comps.append(comp)
        comps_per_dist[source.dist].add(comp)

    sources = []
    sources.extend(self.main_sources)
    sources.extend(self.child_sources)
    # store what comps are enabled already per distro (where distro is
    # e.g. "dapper", "dapper-updates")
    comps_per_dist = {}
    comps_per_sdist = {}
    for s in sources:
      if s.type == self.binary_type: 
        if not comps_per_dist.has_key(s.dist):
          comps_per_dist[s.dist] = set()
        map(comps_per_dist[s.dist].add, s.comps)
    for s in self.source_code_sources:
      if s.type == self.source_type:
        if not comps_per_sdist.has_key(s.dist):
          comps_per_sdist[s.dist] = set()
        map(comps_per_sdist[s.dist].add, s.comps)

    # check if there is a main source at all
    if len(self.main_sources) < 1:
        # create a new main source
        self.add_source(comps=["%s"%comp])
    else:
        # add the comp to all main, child and source code sources
        for source in sources:
             add_component_only_once(source, comps_per_dist)

    # check if there is a main source code source at all
    if self.get_source_code == True:
        if len(self.source_code_sources) < 1:
            # create a new main source
            self.add_source(type=self.source_type, comps=["%s"%comp])
        else:
            # add the comp to all main, child and source code sources
            for source in self.source_code_sources:
                add_component_only_once(source, comps_per_sdist)

  def disable_component(self, comp):
    """
    Disable a component in all main, child and source code sources
    (excluding cdrom based sources)
    """
    sources = []
    sources.extend(self.main_sources)
    sources.extend(self.child_sources)
    sources.extend(self.source_code_sources)
    if comp in self.cdrom_comps:
        sources = []
        sources.extend(self.main_sources)
    for source in sources:
        if comp in source.comps: 
            source.comps.remove(comp)
            if len(source.comps) < 1: 
               self.sourceslist.remove(source)

  def change_server(self, uri):
    ''' Change the server of all distro specific sources to
        a given host '''
    def change_server_of_source(source, uri, seen):
        # Avoid creating duplicate entries
        source.uri = uri
        for comp in source.comps:
            if [source.uri, source.dist, comp] in seen:
                source.comps.remove(comp)
            else:
                seen.append([source.uri, source.dist, comp])
        if len(source.comps) < 1:
            self.sourceslist.remove(source)
    seen_binary = []
    seen_source = []
    self.default_server = uri
    for source in self.main_sources:
        change_server_of_source(source, uri, seen_binary)
    for source in self.child_sources:
        # Do not change the forces server of a child source
        if source.template.base_uri == None or \
           source.template.base_uri != source.uri:
            change_server_of_source(source, uri, seen_binary)
    for source in self.source_code_sources:
        change_server_of_source(source, uri, seen_source)

  def is_codename(self, name):
    ''' Compare a given name with the release codename. '''
    if name == self.codename:
        return True
    else:
        return False

class DebianDistribution(Distribution):
  ''' Class to support specific Debian features '''

  def is_codename(self, name):
    ''' Compare a given name with the release codename and check if
        if it can be used as a synonym for a development releases '''
    if name == self.codename or self.release in ("testing", "unstable"):
        return True
    else:
        return False

  def _get_mirror_name(self, server):
      ''' Try to get a human readable name for the main mirror of a country
          Debian specific '''
      country = None
      i = server.find("://ftp.")
      l = server.find(".debian.org")
      if i != -1 and l != -1:
          country = server[i+len("://ftp."):l]
      if self.countries.has_key(country):
          # TRANSLATORS: %s is a country
          return _("Server for %s") % \
                 gettext.dgettext("iso_3166",
                                  self.countries[country].rstrip()).rstrip()
      else:
          return("%s" % server.rstrip("/ "))

  def get_mirrors(self):
    Distribution.get_mirrors(self,
                             mirror_template="http://ftp.%s.debian.org/debian/")

class UbuntuDistribution(Distribution):
  ''' Class to support specific Ubuntu features '''
  def get_mirrors(self):
    Distribution.get_mirrors(self,
                             mirror_template="http://%s.archive.ubuntu.com/ubuntu/")

def get_distro(id=None,codename=None,description=None,release=None):
    """ 
    Check the currently used distribution and return the corresponding
    distriubtion class that supports distro specific features. 
    
    If no paramter are given the distro will be auto detected via
    a call to lsb-release
    """
    # make testing easier
    if not (id and codename and description and release):
      lsb_info = []
      for lsb_option in ["-i", "-c", "-d", "-r"]:
        pipe = os.popen("lsb_release %s -s" % lsb_option)
        lsb_info.append(pipe.read().strip())
        del pipe
      (id, codename, description, release) = lsb_info
    if id == "Ubuntu":
        return UbuntuDistribution(id, codename, description, 
                                  release)
    elif id == "Debian":
        return DebianDistribution(id, codename, description, release)
    else:
        return Distribution(id, codename, description, release)

