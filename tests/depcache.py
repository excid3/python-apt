#!/usr/bin/env python2.4
#
# Test for the DepCache code
# 

import apt_pkg
import sys

def main():
	apt_pkg.init()
	cache = apt_pkg.GetCache()
	depcache = apt_pkg.GetDepCache(cache)
	depcache.Init()
	i=0
	all=cache.PackageCount
	print "Running DepCache test on all packages"
	print "(trying to install each and then mark it keep again):"
	# first, get all pkgs
	for pkg in cache.Packages:
		i += 1
		x = pkg.Name
		# then get each version
		ver =depcache.GetCandidateVer(pkg)
		if ver != None:
			depcache.MarkInstall(pkg)
			if depcache.InstCount == 0:
				if depcache.IsUpgradable(pkg):
					print "Error marking %s for install" % x
			for p in cache.Packages:
				if depcache.MarkedInstall(p):
					depcache.MarkKeep(p)
			if depcache.InstCount != 0:
				print "Error undoing the selection for %s (InstCount: %s)" % (x,depcache.InstCount)
		print "\r%i/%i=%.3f%%    " % (i,all,(float(i)/float(all)*100)),

	print
	print "Trying Upgrade:"
	depcache.Upgrade()
	print "To install: %s " % depcache.InstCount
	print "To remove: %s " % depcache.DelCount
	print "Kept back: %s " % depcache.KeepCount

	print "Trying DistUpgrade:"
	depcache.Upgrade(True)
	print "To install: %s " % depcache.InstCount
	print "To remove: %s " % depcache.DelCount
	print "Kept back: %s " % depcache.KeepCount


if __name__ == "__main__":
	main()
	sys.exit(0)
