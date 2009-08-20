
import apt_pkg

apt_pkg.init()

sources = apt_pkg.GetPkgSourceList()
sources.ReadMainList()


for metaindex in sources.List:
    print metaindex
    print "URI: ",metaindex.URI
    print "Dist: ",metaindex.Dist
    print "IndexFiles: ","\n".join([str(i) for i in metaindex.IndexFiles])
    print
