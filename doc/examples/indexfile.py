
import apt_pkg

apt_pkg.init()

sources = apt_pkg.GetPkgSourceList()
sources.ReadMainList()

cache = apt_pkg.GetCache()
depcache = apt_pkg.GetDepCache(cache)
pkg = cache["libimlib2"]
cand = depcache.GetCandidateVer(pkg)
for (f,i) in cand.FileList:
    index = sources.FindIndex(f)
    print index
    if index:
        print index.Size
        print index.IsTrusted
        print index.Exists
        print index.HasPackages
        print index.ArchiveURI("some/path")
