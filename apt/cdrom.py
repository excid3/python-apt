import apt_pkg
from progress import CdromProgress

class Cdrom(object):
    def __init__(self, progress=None, mountpoint=None, nomount=True):
        """ Support for apt-cdrom like features.
            Options:
            - progress: optional progress.CdromProgress() subclass
            - mountpoint: optional alternative mountpoint
            - nomount: do not mess with mount/umount the CD
        """
        self._cdrom = apt_pkg.GetCdrom()
        if progress is None:
            self._progress = CdromProgress()
        else:
            self._progress = progress
        # see if we have a alternative mountpoint
        if mountpoint is not None:
            apt_pkg.Config.Set("Acquire::cdrom::mount",mountpoint)
        # do not mess with mount points by default
        if nomount is True:
            apt_pkg.Config.Set("APT::CDROM::NoMount", "true")
        else:
            apt_pkg.Config.Set("APT::CDROM::NoMount", "false")
    def add(self):
        " add cdrom to the sources.list "
        return self._cdrom.Add(self._progress)
    def ident(self):
        " identify the cdrom "
        (res, ident) = self._cdrom.Ident(self._progress)
        if res:
            return ident
        return None
    @property
    def inSourcesList(self):
        " check if the cdrom is already in the current sources.list "
        cdid = self.ident()
        if cdid is None:
            # FIXME: throw exception instead
            return False
        # FIXME: check sources.list.d/ as well
        for line in open(apt_pkg.Config.FindFile("Dir::Etc::sourcelist")):
            line = line.strip()
            if not line.startswith("#") and cdid in line:
                return True
        return False
        
