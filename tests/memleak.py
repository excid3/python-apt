#!/usr/bin/python

import apt
import apt_pkg
import time
import gc
import sys


cache = apt.Cache()

# memleak
for i in range(100):
	cache.open(None)
	print cache["apt"].name
	time.sleep(1)
	gc.collect()
	f = open("%s" % i,"w")
	for obj in gc.get_objects():
		f.write("%s\n" % str(obj))
	f.close()

# memleak	
#for i in range(100):
#	cache = apt.Cache()
#	time.sleep(1)
#	cache = None
#	gc.collect()

# no memleak, but more or less the apt.Cache.open() code
for i in range(100):
	cache = apt_pkg.GetCache()
	depcache = apt_pkg.GetDepCache(cache)
	records = apt_pkg.GetPkgRecords(cache)
	list = apt_pkg.GetPkgSourceList()
	list.ReadMainList()
	dict = {}
	for pkg in cache.Packages:
            if len(pkg.VersionList) > 0:
		    dict[pkg.Name] = apt.Package(cache,depcache,
						 records, list, None, pkg)

	print cache["apt"]
	time.sleep(1)

	gc.collect()
