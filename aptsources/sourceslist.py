#  aptsource.py - Provide an abstraction of the sources.list
#  
#  Copyright (c) 2004-2007 Canonical Ltd.
#                2004 Michiel Sikkes
#                2006-2007 Sebastian Heinlein
#  
#  Author: Michiel Sikkes <michiel@eyesopened.nl>
#          Michael Vogt <mvo@debian.org>
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
 
import string
import gettext
import re
import apt_pkg
import glob
import shutil
import time
import os.path
import sys

#from UpdateManager.Common.DistInfo import DistInfo
from distinfo import DistInfo

# some global helpers
def is_mirror(master_uri, compare_uri):
  """check if the given add_url is idential or a mirror of orig_uri
    e.g. master_uri = archive.ubuntu.com
      compare_uri = de.archive.ubuntu.com
      -> True
  """
  # remove traling spaces and "/"
  compare_uri = compare_uri.rstrip("/ ")
  master_uri = master_uri.rstrip("/ ")
  # uri is identical
  if compare_uri == master_uri:
    #print "Identical"
    return True
  # add uri is a master site and orig_uri has the from "XX.mastersite"
  # (e.g. de.archive.ubuntu.com)
  try:
    compare_srv = compare_uri.split("//")[1]
    master_srv = master_uri.split("//")[1]
    #print "%s == %s " % (add_srv, orig_srv)
  except IndexError: # ok, somethings wrong here
    #print "IndexError"
    return False
  # remove the leading "<country>." (if any) and see if that helps
  if "." in compare_srv and \
         compare_srv[compare_srv.index(".")+1:] == master_srv:
    #print "Mirror"
    return True
  return False

def uniq(s):
  """ simple and efficient way to return uniq list """
  return list(set(s))

class SourceEntry:
  """ single sources.list entry """
  def __init__(self, line,file=None):
    self.invalid = False            # is the source entry valid
    self.disabled = False           # is it disabled ('#' in front)
    self.type = ""                  # what type (deb, deb-src)
    self.uri = ""                   # base-uri
    self.dist = ""                  # distribution (dapper, edgy, etc)
    self.comps = []                 # list of available componetns (may empty)
    self.comment = ""               # (optional) comment
    self.line = line                # the original sources.list line
    if file == None:                
      file = apt_pkg.Config.FindDir("Dir::Etc")+apt_pkg.Config.Find("Dir::Etc::sourcelist")
    self.file = file               # the file that the entry is located in
    self.parse(line)
    self.template = None           # type DistInfo.Suite
    self.children = []

  def __eq__(self, other):         
    """ equal operator for two sources.list entries """
    return (self.disabled == other.disabled and
            self.type == other.type and
            self.uri == other.uri and
            self.dist == other.dist and
            self.comps == other.comps)


  def mysplit(self, line):
    """ a split() implementation that understands the sources.list
        format better and takes [] into account (for e.g. cdroms) """
    line = string.strip(line)
    pieces = []
    tmp = ""
    # we are inside a [..] block
    p_found = False
    space_found = False
    for i in range(len(line)):
      if line[i] == "[":
        p_found=True
        tmp += line[i]
      elif line[i] == "]":
        p_found=False
        tmp += line[i]
      elif space_found and not line[i].isspace(): # we skip one or more space
        space_found = False
        pieces.append(tmp)
        tmp = line[i]
      elif line[i].isspace() and not p_found:     # found a whitespace
        space_found = True
      else:
        tmp += line[i]
    # append last piece
    if len(tmp) > 0:
      pieces.append(tmp)
    return pieces

  def parse(self,line):
    """ parse a given sources.list (textual) line and break it up
        into the field we have """
    line  = string.strip(self.line)
    #print line
    # check if the source is enabled/disabled
    if line == "" or line == "#": # empty line
      self.invalid = True
      return
    if line[0] == "#":
      self.disabled = True
      pieces = string.split(line[1:])
      # if it looks not like a disabled deb line return 
      if not pieces[0] in ("rpm", "rpm-src", "deb", "deb-src"):
        self.invalid = True
        return
      else:
        line = line[1:]
    # check for another "#" in the line (this is treated as a comment)
    i = line.find("#")
    if i > 0:
      self.comment = line[i+1:]
      line = line[:i]
    # source is ok, split it and see what we have
    pieces = self.mysplit(line)
    # Sanity check
    if len(pieces) < 3:
        self.invalid = True
        return
    # Type, deb or deb-src
    self.type = string.strip(pieces[0])
    # Sanity check
    if self.type not in ("deb", "deb-src", "rpm", "rpm-src"):
      self.invalid = True
      return
    # URI
    self.uri = string.strip(pieces[1])
    if len(self.uri) < 1:
      self.invalid = True
    # distro and components (optional)
    # Directory or distro
    self.dist = string.strip(pieces[2])
    if len(pieces) > 3:
      # List of components
      self.comps = pieces[3:]
    else:
      self.comps = []

  def set_enabled(self, new_value):
    """ set a line to enabled or disabled """
    self.disabled = not new_value
    # enable, remove all "#" from the start of the line
    if new_value == True:
      i=0
      self.line = string.lstrip(self.line)
      while self.line[i] == "#":
        i += 1
      self.line = self.line[i:]
    else:
      # disabled, add a "#" 
      if string.strip(self.line)[0] != "#":
        self.line = "#" + self.line

  def __str__(self):
    """ debug helper """
    return self.str().strip()

  def str(self):
    """ return the current line as string """
    if self.invalid:
      return self.line
    line = ""
    if self.disabled:
      line = "# "
    line += "%s %s %s" % (self.type, self.uri, self.dist)
    if len(self.comps) > 0:
      line += " " + " ".join(self.comps)
    if self.comment != "":
      line += " #"+self.comment
    line += "\n"
    return line
    
class NullMatcher(object):
  """ a Matcher that does nothing """
  def match(self, s):
    return True

class SourcesList:
  """ represents the full sources.list + sources.list.d file """
  def __init__(self,
               withMatcher=True,
               matcherPath="/usr/share/python-apt/templates/"):
    self.list = []          # the actual SourceEntries Type 
    if withMatcher:
      self.matcher = SourceEntryMatcher(matcherPath)
    else:
      self.matcher = NullMatcher()
    self.refresh()

  def refresh(self):
    """ update the list of known entries """
    self.list = []
    # read sources.list
    dir = apt_pkg.Config.FindDir("Dir::Etc")
    file = apt_pkg.Config.Find("Dir::Etc::sourcelist")
    self.load(dir+file)
    # read sources.list.d
    partsdir = apt_pkg.Config.FindDir("Dir::Etc::sourceparts")
    for file in glob.glob("%s/*.list" % partsdir):
      self.load(file)
    # check if the source item fits a predefined template
    for source in self.list:
        if source.invalid == False:
            self.matcher.match(source)

  def __iter__(self):
    """ simple iterator to go over self.list, returns SourceEntry
        types """
    for entry in self.list:
      yield entry
    raise StopIteration

  def add(self, type, uri, dist, orig_comps, comment="", pos=-1, file=None):
    """
    Add a new source to the sources.list.
    The method will search for existing matching repos and will try to 
    reuse them as far as possible
    """
    # create a working copy of the component list so that
    # we can modify it later
    comps = orig_comps[:]
    # check if we have this source already in the sources.list
    for source in self.list:
      if source.disabled == False and source.invalid == False and \
         source.type == type and uri == source.uri and \
         source.dist == dist:
        for new_comp in comps:
          if new_comp in source.comps:
            # we have this component already, delete it from the new_comps
            # list
            del comps[comps.index(new_comp)]
            if len(comps) == 0:
              return source
    for source in self.list:
      # if there is a repo with the same (type, uri, dist) just add the
      # components
      if source.disabled == False and source.invalid == False and \
         source.type == type and uri == source.uri and \
         source.dist == dist:
        comps = uniq(source.comps + comps)
        source.comps = comps
        return source
      # if there is a corresponding repo which is disabled, enable it
      elif source.disabled == True and source.invalid == False and \
           source.type == type and uri == source.uri and \
           source.dist == dist and \
           len(set(source.comps) & set(comps)) == len(comps):
        source.disabled = False
        return source
    # there isn't any matching source, so create a new line and parse it
    line = "%s %s %s" % (type,uri,dist)
    for c in comps:
      line = line + " " + c;
    if comment != "":
      line = "%s #%s\n" %(line,comment)
    line = line + "\n"
    new_entry = SourceEntry(line)
    if file != None:
      new_entry.file = file
    self.matcher.match(new_entry)
    self.list.insert(pos, new_entry)
    return new_entry

  def remove(self, source_entry):
    """ remove the specified entry from the sources.list """
    self.list.remove(source_entry)

  def restoreBackup(self, backup_ext):
    " restore sources.list files based on the backup extension "
    dir = apt_pkg.Config.FindDir("Dir::Etc")
    file = apt_pkg.Config.Find("Dir::Etc::sourcelist")
    if os.path.exists(dir+file+backup_ext) and \
       os.path.exists(dir+file):
      shutil.copy(dir+file+backup_ext,dir+file)
    # now sources.list.d
    partsdir = apt_pkg.Config.FindDir("Dir::Etc::sourceparts")
    for file in glob.glob("%s/*.list" % partsdir):
      if os.path.exists(file+backup_ext):
        shutil.copy(file+backup_ext,file)

  def backup(self, backup_ext=None):
    """ make a backup of the current source files, if no backup extension
        is given, the current date/time is used (and returned) """
    already_backuped = set()
    if backup_ext == None:
      backup_ext = time.strftime("%y%m%d.%H%M")
    for source in self.list:
      if not source.file in already_backuped and os.path.exists(source.file):
        shutil.copy(source.file,"%s%s" % (source.file,backup_ext))
    return backup_ext

  def load(self,file):
    """ (re)load the current sources """
    try:
      f = open(file, "r")
      lines = f.readlines()
      for line in lines:
        source = SourceEntry(line,file)
        self.list.append(source)
    except:
      print "could not open file '%s'" % file
    else:
      f.close()

  def save(self):
    """ save the current sources """
    files = {}
    # write an empty default config file if there aren't any sources
    if len(self.list) == 0:
      path = "%s%s" % (apt_pkg.Config.FindDir("Dir::Etc"),
                       apt_pkg.Config.Find("Dir::Etc::sourcelist"))
      header = ("## See sources.list(5) for more information, especialy\n"
                "# Remember that you can only use http, ftp or file URIs\n"
                "# CDROMs are managed through the apt-cdrom tool.\n")
      open(path,"w").write(header)
      return
    for source in self.list:
      if not files.has_key(source.file):
        files[source.file]=open(source.file,"w")
      files[source.file].write(source.str())
    for f in files:
      files[f].close()

  def check_for_relations(self, sources_list):
    """get all parent and child channels in the sources list"""
    parents = []
    used_child_templates = {}
    for source in sources_list:
      # try to avoid checking uninterressting sources
      if source.template == None:
        continue
      # set up a dict with all used child templates and corresponding 
      # source entries
      if source.template.child == True:
          key = source.template
          if not used_child_templates.has_key(key):
              used_child_templates[key] = []
          temp = used_child_templates[key]
          temp.append(source)
      else:
          # store each source with children aka. a parent :)
          if len(source.template.children) > 0:
              parents.append(source)
    #print self.used_child_templates
    #print self.parents
    return (parents, used_child_templates)

# matcher class to make a source entry look nice
# lots of predefined matchers to make it i18n/gettext friendly
class SourceEntryMatcher:
  def __init__(self, matcherPath):
    self.templates = []
    # Get the human readable channel and comp names from the channel .infos
    spec_files = glob.glob("%s/*.info" % matcherPath)
    for f in spec_files:
        f = os.path.basename(f)
        i = f.find(".info")
        f = f[0:i]
        dist = DistInfo(f,base_dir=matcherPath)
        for template in dist.templates:
            if template.match_uri != None:
                self.templates.append(template)
    return

  def match(self, source):
    """Add a matching template to the source"""
    _ = gettext.gettext
    found = False
    for template in self.templates:
      if (re.search(template.match_uri, source.uri) and 
          re.match(template.match_name, source.dist)):
        found = True
        source.template = template
        break
      elif (template.is_mirror(source.uri) and 
          re.match(template.match_name, source.dist)):
        found = True
        source.template = template
        break
    return found


# some simple tests
if __name__ == "__main__":
  apt_pkg.InitConfig()
  sources = SourcesList()

  for entry in sources:
    print entry.str()
    #print entry.uri

  mirror = is_mirror("http://archive.ubuntu.com/ubuntu/",
                     "http://de.archive.ubuntu.com/ubuntu/")
  print "is_mirror(): %s" % mirror
  
  print is_mirror("http://archive.ubuntu.com/ubuntu",
                  "http://de.archive.ubuntu.com/ubuntu/")
  print is_mirror("http://archive.ubuntu.com/ubuntu/",
                  "http://de.archive.ubuntu.com/ubuntu")

