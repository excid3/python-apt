# Progress.py - progress reporting classes
#  
#  Copyright (c) 2005 Canonical
#  
#  Author: Michael Vogt <michael.vogt@ubuntu.com>
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

import sys
import os
import re
import fcntl
import string
from errno import *
import select
import apt_pkg

import apt

class OpProgress(object):
    """ Abstract class to implement reporting on cache opening
        Subclass this class to implement simple Operation progress reporting
    """
    def __init__(self):
        pass
    def update(self, percent):
        pass
    def done(self):
        pass

class OpTextProgress(OpProgress):
    """ A simple text based cache open reporting class """
    def __init__(self):
        OpProgress.__init__(self)
    def update(self, percent):
        sys.stdout.write("\r%s: %.2i  " % (self.subOp,percent))
        sys.stdout.flush()
    def done(self):
        sys.stdout.write("\r%s: Done\n" % self.op)



class FetchProgress(object):
    """ Report the download/fetching progress
        Subclass this class to implement fetch progress reporting
    """

    # download status constants
    dlDone = 0
    dlQueued = 1
    dlFailed = 2
    dlHit = 3
    dlIgnored = 4
    dlStatusStr = {dlDone : "Done",
                   dlQueued : "Queued",
                   dlFailed : "Failed",
                   dlHit : "Hit",
                   dlIgnored : "Ignored"}
    
    def __init__(self):
        self.eta = 0.0
        self.percent = 0.0
        pass
    
    def start(self):
        pass
    
    def stop(self):
        pass
    
    def updateStatus(self, uri, descr, shortDescr, status):
        pass

    def pulse(self):
        """ called periodically (to update the gui), importend to
            return True to continue or False to cancel
        """
        self.percent = ((self.currentBytes + self.currentItems)*100.0)/float(self.totalBytes+self.totalItems)
        if self.currentCPS > 0:
            self.eta = (self.totalBytes-self.currentBytes)/float(self.currentCPS)
        return True
    def mediaChange(self, medium, drive):
        pass

class TextFetchProgress(FetchProgress):
    """ Ready to use progress object for terminal windows """
    def __init__(self):
        self.items = {}
    def updateStatus(self, uri, descr, shortDescr, status):
        if status != self.dlQueued:
            print "\r%s %s" % (self.dlStatusStr[status], descr)
        self.items[uri] = status
    def pulse(self):
        FetchProgress.pulse(self)
        if self.currentCPS > 0:
            s = "[%2.f%%] %sB/s %s" % (self.percent,
                                       apt_pkg.SizeToStr(int(self.currentCPS)),
                                       apt_pkg.TimeToStr(int(self.eta)))
        else:
            s = "%2.f%% [Working]" % (self.percent)
        print "\r%s" % (s),
        sys.stdout.flush()
        return True
    def stop(self):
        print "\rDone downloading            " 
    def mediaChange(self, medium, drive):
        """ react to media change events """
        res = True;
        print "Media change: please insert the disc labeled \
               '%s' in the drive '%s' and press enter" % (medium,drive)
        s = sys.stdin.readline()
        if(s == 'c' or s == 'C'):
            res = false;
        return res

class DumbInstallProgress(object):
    """ Report the install progress
        Subclass this class to implement install progress reporting
    """
    def __init__(self):
        pass
    def startUpdate(self):
        pass
    def run(self, pm):
        return pm.DoInstall()
    def finishUpdate(self):
        pass
    def updateInterface(self):
        pass

class InstallProgress(DumbInstallProgress):
    """ A InstallProgress that is pretty useful.
        It supports the attributes 'percent' 'status' and callbacks
        for the dpkg errors and conffiles and status changes 
     """
    def __init__(self):
        DumbInstallProgress.__init__(self)
        self.selectTimeout = 0.1
        (read, write) = os.pipe()
        self.writefd=write
        self.statusfd = os.fdopen(read, "r")
        fcntl.fcntl(self.statusfd.fileno(), fcntl.F_SETFL,os.O_NONBLOCK)
        self.read = ""
        self.percent = 0.0
        self.status = ""
    def error(self, pkg, errormsg):
        " called when a error is detected during the install "
        pass
    def conffile(self,current,new):
        " called when a conffile question from dpkg is detected "
        pass
    def statusChange(self, pkg, percent, status):
	" called when the status changed "
	pass
    def updateInterface(self):
        if self.statusfd != None:
                try:
		    while not self.read.endswith("\n"):
	                    self.read += os.read(self.statusfd.fileno(),1)
                except OSError, (errno,errstr):
                    # resource temporarly unavailable is ignored
                    if errno != EAGAIN and errnor != EWOULDBLOCK:
                        print errstr
                if self.read.endswith("\n"):
                    s = self.read
                    #print s
                    try:
                        (status, pkg, percent, status_str) = string.split(s, ":",3)
                    except ValueError, e:
                        # silently ignore lines that can't be parsed
                        self.read = ""
                        return
                    #print "percent: %s %s" % (pkg, float(percent)/100.0)
                    if status == "pmerror":
                        self.error(pkg,status_str)
                    elif status == "pmconffile":
                        # we get a string like this:
                        # 'current-conffile' 'new-conffile' useredited distedited
                        match = re.compile("\s*\'(.*)\'\s*\'(.*)\'.*").match(status_str)
                        if match:
                            self.conffile(match.group(1), match.group(2))
                    elif status == "pmstatus":
                        if float(percent) != self.percent or \
                           status_str != self.status:
                            self.statusChange(pkg, float(percent), status_str.strip())
                        self.percent = float(percent)
                        self.status = string.strip(status_str)
                    self.read = ""
    def fork(self):
        return os.fork()
    def waitChild(self):
        while True:
            select.select([self.statusfd],[],[], self.selectTimeout)
            self.updateInterface()
            (pid, res) = os.waitpid(self.child_pid,os.WNOHANG)
            if pid == self.child_pid:
                break
        return os.WEXITSTATUS(res)
    def run(self, pm):
        pid = self.fork()
        if pid == 0:
            # child
            res = pm.DoInstall(self.writefd)
            os._exit(res)
        self.child_pid = pid
        res = self.waitChild()
        return res

class CdromProgress:
    """ Report the cdrom add progress
        Subclass this class to implement cdrom add progress reporting
    """
    def __init__(self):
        pass
    def update(self, text, step):
        """ update is called regularly so that the gui can be redrawn """
        pass
    def askCdromName(self):
        pass
    def changeCdrom(self):
        pass

# module test code
if __name__ == "__main__":
    import apt_pkg
    apt_pkg.init()
    progress = OpTextProgress()
    cache = apt_pkg.GetCache(progress)
    depcache = apt_pkg.GetDepCache(cache)
    depcache.Init(progress)

    fprogress = TextFetchProgress()
    cache.Update(fprogress)
