import apt_inst
import apt_pkg
from apt_inst import arCheckMember

from gettext import gettext as _

class NoDebArchiveException(IOError):
    pass

class DebPackage(object):

    _supported_data_members = ("data.tar.gz", "data.tar.bz2", "data.tar.lzma")

    def __init__(self, filename=None):
        self._section = {}
        if filename:
            self.open(filename)

    def open(self, filename):
        " open given debfile "
        self.filename = filename
        if not arCheckMember(open(self.filename), "debian-binary"):
            raise NoDebArchiveException, _("This is not a valid DEB archive, missing '%s' member" % "debian-binary")
        control = apt_inst.debExtractControl(open(self.filename))
        self._sections = apt_pkg.ParseSection(control)
        self.pkgname = self._sections["Package"]

    def __getitem__(self, key):
        return self._sections[key]
        
    def filelist(self):
        """ return the list of files in the deb """
        files = []
        def extract_cb(What,Name,Link,Mode,UID,GID,Size,MTime,Major,Minor):
            #print "%s '%s','%s',%u,%u,%u,%u,%u,%u,%u"\
            #      % (What,Name,Link,Mode,UID,GID,Size, MTime, Major, Minor)
            files.append(Name)
        for member in self._supported_data_members:
            if arCheckMember(open(self.filename), member):
                try:
                    apt_inst.debExtract(open(self.filename), extract_cb, member)
                    break
                except SystemError, e:
                    return [_("List of files for '%s'could not be read" % self.filename)]
        return files
    filelist = property(filelist)



if __name__ == "__main__":
    import sys
    
    d = DebPackage(sys.argv[1])
    print d["Section"]
    print d["Maintainer"]
    print "Files:"
    print "\n".join(d.filelist)
    
