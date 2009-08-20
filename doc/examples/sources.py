#!/usr/bin/python

import apt_pkg

apt_pkg.init()

#cache = apt_pkg.GetCache()
#sources = apt_pkg.GetPkgSrcRecords(cache)

sources = apt_pkg.GetPkgSrcRecords()
sources.Restart()
while sources.Lookup('hello'):
	print sources.Package, sources.Version, sources.Maintainer, sources.Section, `sources.Binaries`
	print sources.Files
	print sources.Index.ArchiveURI(sources.Files[0][2])
