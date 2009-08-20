#!/usr/bin/python
# this is a example how to access the build dependencies of a package

import apt_pkg
import sys
import sets    # only needed for python2.3, python2.4 supports this naively 

def get_source_pkg(pkg, records, depcache):
	""" get the source package name of a given package """
	version = depcache.GetCandidateVer(pkg)
	if not version:
		return None
	file, index = version.FileList.pop(0)
	records.Lookup((file, index))
	if records.SourcePkg != "":
		srcpkg = records.SourcePkg
	else:
		srcpkg = pkg.Name
	return srcpkg

# main
apt_pkg.init()
cache = apt_pkg.GetCache()
depcache = apt_pkg.GetDepCache(cache)
depcache.Init()
records = apt_pkg.GetPkgRecords(cache)
srcrecords = apt_pkg.GetPkgSrcRecords()

# base package that we use for build-depends calculation
if len(sys.argv) < 2:
	print "need a package name as argument"
	sys.exit(1)
try:
	base = cache[sys.argv[1]]
except KeyError:
	print "No package %s found" % sys.argv[1]
	sys.exit(1)
all_build_depends = sets.Set()

# get the build depdends for the package itself
srcpkg_name = get_source_pkg(base, records, depcache)
print "srcpkg_name: %s " % srcpkg_name
if not srcpkg_name:
	print "Can't find source package for '%s'" % pkg.Name
srcrec = srcrecords.Lookup(srcpkg_name)
if srcrec:
	print "Files:"
	print srcrecords.Files
	bd = srcrecords.BuildDepends
	print "build-depends of the package: %s " % bd
        for b in bd:
        	all_build_depends.add(b[0])

# calculate the build depends for all dependencies
depends = depcache.GetCandidateVer(base).DependsList
for dep in depends["Depends"]: # FIXME: do we need to consider PreDepends?
	pkg = dep[0].TargetPkg
	srcpkg_name = get_source_pkg(pkg, records, depcache)
	if not srcpkg_name:
		print "Can't find source package for '%s'" % pkg.Name
		continue
	srcrec = srcrecords.Lookup(srcpkg_name)
	if srcrec:
		#print srcrecords.Package
		#print srcrecords.Binaries
		bd = srcrecords.BuildDepends
		#print "%s: %s " % (srcpkg_name, bd)
		for b in bd:
			all_build_depends.add(b[0])
			

print "\n".join(all_build_depends)
