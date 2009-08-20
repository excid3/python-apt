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
			if depcache.BrokenCount > 0:
				fixer = apt_pkg.GetPkgProblemResolver(depcache)
				fixer.Clear(pkg)
				fixer.Protect(pkg)
				# we first try to resolve the problem
				# with the package that should be installed
				# protected 
				try:
					fixer.Resolve(True)
				except SystemError:
					# the pkg seems to be broken, the
					# returns a exception
					fixer.Clear(pkg)
					fixer.Resolve(True)
					if not depcache.MarkedInstall(pkg):
						print "broken in archive: %s " % pkg.Name
				fixer = None
			if depcache.InstCount == 0:
				if depcache.IsUpgradable(pkg):
					print "Error marking %s for install" % x
			for p in cache.Packages:
				if depcache.MarkedInstall(p) or depcache.MarkedUpgrade(p):
					depcache.MarkKeep(p)
			if depcache.InstCount != 0:
				print "Error undoing the selection for %s" % x
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
