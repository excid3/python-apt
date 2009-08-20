import apt
from apt import SizeToStr
import sys
import time
import string

class TextProgress(apt.OpProgress):
    def __init__(self):
        self.last=0.0

    def update(self, percent):
        if (self.last + 1.0) <= percent:
            sys.stdout.write("\rProgress: %i.2          " % (percent))
            self.last = percent
        if percent >= 100:
            self.last = 0.0

    def done(self):
        self.last = 0.0
        print "\rDone                      "


class TextFetchProgress(apt.FetchProgress):
    def __init__(self):
        pass
    
    def start(self):
        pass
    
    def stop(self):
        pass
    
    def updateStatus(self, uri, descr, shortDescr, status):
        print "UpdateStatus: '%s' '%s' '%s' '%i'" % (uri,descr,shortDescr, status)
    def pulse(self):
        print "Pulse: CPS: %s/s; Bytes: %s/%s; Item: %s/%s" % (SizeToStr(self.currentCPS), SizeToStr(self.currentBytes), SizeToStr(self.totalBytes), self.currentItems, self.totalItems)
        return True

    def mediaChange(self, medium, drive):
	print "Please insert medium %s in drive %s" % (medium, drive)
	sys.stdin.readline()
        #return False


class TextInstallProgress(apt.InstallProgress):
    def __init__(self):
        apt.InstallProgress.__init__(self)
        pass
    def startUpdate(self):
        print "StartUpdate"
    def finishUpdate(self):
        print "FinishUpdate"
    def statusChange(self, pkg, percent, status):
        print "[%s] %s: %s" % (percent, pkg, status)
    def updateInterface(self):
        apt.InstallProgress.updateInterface(self)
        # usefull to e.g. redraw a GUI
        time.sleep(0.1)


class TextCdromProgress(apt.CdromProgress):
    def __init__(self):
        pass
    # update is called regularly so that the gui can be redrawn
    def update(self, text, step):
        # check if we actually have some text to display
        if text != "":
            print "Update: %s %s" % (string.strip(text), step)
    def askCdromName(self):
        print "Please enter cd-name: ",
        cd_name = sys.stdin.readline()
        return (True, string.strip(cd_name))
    def changeCdrom(self):
        print "Please insert cdrom and press <ENTER>"
        answer =  sys.stdin.readline()
        return True


if __name__ == "__main__":
    c = apt.Cache()
    pkg = c["3dchess"]
    if pkg.isInstalled:
        pkg.markDelete()
    else:
        pkg.markInstall()

    res = c.commit(TextFetchProgress(), TextInstallProgress())

    print res
