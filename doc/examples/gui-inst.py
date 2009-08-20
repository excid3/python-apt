#!/usr/bin/python
# example how to install in a custom terminal widget
# see also gnome bug: #169201

import apt
import apt_pkg
import sys, os, fcntl
import copy
import string
import fcntl

import pygtk
pygtk.require('2.0')
import gtk
import vte
import time
import posix

from apt.progress import OpProgress, FetchProgress, InstallProgress

class GuiFetchProgress(gtk.Window, FetchProgress):
    def __init__(self):
	gtk.Window.__init__(self)
	self.vbox = gtk.VBox()
	self.vbox.show()
	self.add(self.vbox)
	self.progress = gtk.ProgressBar()
	self.progress.show()
	self.label = gtk.Label()
	self.label.show()
	self.vbox.pack_start(self.progress)
	self.vbox.pack_start(self.label)
	self.resize(300,100)
    def start(self):
        print "start"
	self.progress.set_fraction(0.0)
        self.show()
    def stop(self):
	self.hide()
    def pulse(self):
        FetchProgress.pulse(self)
        self.label.set_text("Speed: %s/s" % apt_pkg.SizeToStr(self.currentCPS))
	#self.progressbar.set_fraction(self.currentBytes/self.totalBytes)
	while gtk.events_pending():
		gtk.main_iteration()
        return True

class TermInstallProgress(InstallProgress, gtk.Window):
    def __init__(self):
	gtk.Window.__init__(self)
        InstallProgress.__init__(self)
	self.show()
        box = gtk.VBox()
        box.show()
        self.add(box)
	self.term = vte.Terminal()
	self.term.show()
        # check for the child
        self.reaper = vte.reaper_get()
        self.reaper.connect("child-exited",self.child_exited)
        self.finished = False
	box.pack_start(self.term)
        self.progressbar = gtk.ProgressBar()
        self.progressbar.show()
        box.pack_start(self.progressbar)
    def child_exited(self,term, pid, status):
        print "child_exited: %s %s %s %s" % (self,term,pid,status)
        self.apt_status = posix.WEXITSTATUS(status)
        self.finished = True
    def startUpdate(self):
        print "start"
        self.show()
    def waitChild(self):
        while not self.finished:
            self.updateInterface()
            while gtk.events_pending():
                gtk.main_iteration()
            time.sleep(0.001)
        sys.stdin.readline()
        return self.apt_status
    def statusChange(self, pkg, percent, status):
        print "statusChange", pkg, percent
        self.progressbar.set_fraction(float(percent)/100.0)
        self.progressbar.set_text(string.strip(status))
    def fork(self):
        env = ["VTE_PTY_KEEP_FD=%s"%self.writefd]
        return self.term.forkpty(envv=env)

cache = apt.Cache()
print "Available packages: %s " % cache._cache.PackageCount


# update the cache
fprogress = GuiFetchProgress()
iprogress = TermInstallProgress()

# update the cache
#cache.Update(fprogress)
#cache = apt_pkg.GetCache(progress)
#depcache = apt_pkg.GetDepCache(cache)
#depcache.ReadPinFile()
#depcache.Init(progress)


# show the interface
while gtk.events_pending():
	gtk.main_iteration()
  

pkg = cache["3dchess"]
print "\n%s"%pkg.name

# install or remove, the importend thing is to keep us busy :)
if pkg.isInstalled:
	pkg.markDelete()
else:
	pkg.markInstall()
cache.commit(fprogress, iprogress)

print "Exiting"
sys.exit(0)



