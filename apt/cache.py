# cache.py - apt cache abstraction
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

import apt_pkg
from apt import Package
import apt.progress
import os
import sys

class FetchCancelledException(IOError):
    " Exception that is thrown when the user cancels a fetch operation "
    pass
class FetchFailedException(IOError):
    " Exception that is thrown when fetching fails "
    pass
class LockFailedException(IOError):
    " Exception that is thrown when locking fails "
    pass

class Cache(object):
    """ Dictionary-like package cache 
        This class has all the packages that are available in it's
        dictionary
    """

    def __init__(self, progress=None, rootdir=None, memonly=False):
        self._callbacks = {}
        if memonly:
            # force apt to build its caches in memory
            apt_pkg.Config.Set("Dir::Cache::pkgcache","")
        if rootdir:
            apt_pkg.Config.Set("Dir", rootdir)
            apt_pkg.Config.Set("Dir::State::status", rootdir + "/var/lib/dpkg/status")
        self.open(progress)

    def _runCallbacks(self, name):
        """ internal helper to run a callback """
        if self._callbacks.has_key(name):
            for callback in self._callbacks[name]:
                callback()
        
    def open(self, progress):
        """ Open the package cache, after that it can be used like
            a dictionary
        """
        self._runCallbacks("cache_pre_open")
        self._cache = apt_pkg.GetCache(progress)
        self._depcache = apt_pkg.GetDepCache(self._cache)
        self._records = apt_pkg.GetPkgRecords(self._cache)
        self._list = apt_pkg.GetPkgSourceList()
        self._list.ReadMainList()
        self._dict = {}

        # build the packages dict
        if progress != None:
            progress.Op = "Building data structures"
        i=last=0
        size=len(self._cache.Packages)
        for pkg in self._cache.Packages:
            if progress != None and last+100 < i:
                progress.update(i/float(size)*100)
                last=i
            # drop stuff with no versions (cruft)
            if len(pkg.VersionList) > 0:
                self._dict[pkg.Name] = Package(self._cache, self._depcache,
                                               self._records, self._list,
                                               self, pkg)
                
            i += 1
        if progress != None:
            progress.done()
        self._runCallbacks("cache_post_open")
        
    def __getitem__(self, key):
        """ look like a dictionary (get key) """
        return self._dict[key]

    def __iter__(self):
        for pkgname in self._dict.keys():
            yield self._dict[pkgname]
        raise StopIteration

    def has_key(self, key):
        return self._dict.has_key(key)

    def __contains__(self, key):
        return key in self._dict

    def __len__(self):
        return len(self._dict)

    def keys(self):
        return self._dict.keys()

    def getChanges(self):
        """ Get the marked changes """
        changes = [] 
        for name in self._dict.keys():
            p = self._dict[name]
            if p.markedUpgrade or p.markedInstall or p.markedDelete or \
               p.markedDowngrade or p.markedReinstall:
                changes.append(p)
        return changes

    def upgrade(self, distUpgrade=False):
        """ Upgrade the all package, DistUpgrade will also install
            new dependencies
        """
        self.cachePreChange()
        self._depcache.Upgrade(distUpgrade)
        self.cachePostChange()

    @property
    def reqReinstallPkgs(self):
        " return the packages not downloadable packages in reqreinst state "
        reqreinst = set()
        for pkg in self:
            if (not pkg.candidateDownloadable and 
                (pkg._pkg.InstState == apt_pkg.InstStateReInstReq or
                 pkg._pkg.InstState == apt_pkg.InstStateHoldReInstReq)):
                reqreinst.add(pkg.name)
        return reqreinst

    def _runFetcher(self, fetcher):
        # do the actual fetching
        res = fetcher.Run()
        
        # now check the result (this is the code from apt-get.cc)
        failed = False
        transient = False
        errMsg = ""
        for item in fetcher.Items:
            if item.Status == item.StatDone:
                continue
            if item.StatIdle:
                transient = True
                continue
            errMsg += "Failed to fetch %s %s\n" % (item.DescURI,item.ErrorText)
            failed = True

        # we raise a exception if the download failed or it was cancelt
        if res == fetcher.ResultCancelled:
            raise FetchCancelledException, errMsg
        elif failed:
            raise FetchFailedException, errMsg
        return res

    def _fetchArchives(self, fetcher, pm):
        """ fetch the needed archives """

        # get lock
        lockfile = apt_pkg.Config.FindDir("Dir::Cache::Archives") + "lock"
        lock = apt_pkg.GetLock(lockfile)
        if lock < 0:
            raise LockFailedException, "Failed to lock %s" % lockfile

        try:
            # this may as well throw a SystemError exception
            if not pm.GetArchives(fetcher, self._list, self._records):
                return False
            # now run the fetcher, throw exception if something fails to be
            # fetched
            return self._runFetcher(fetcher)
        finally:
            os.close(lock)

    def update(self, fetchProgress=None):
        " run the equivalent of apt-get update "
        lockfile = apt_pkg.Config.FindDir("Dir::State::Lists") + "lock"
        lock = apt_pkg.GetLock(lockfile)
        if lock < 0:
            raise LockFailedException, "Failed to lock %s" % lockfile

        try:
            if fetchProgress == None:
                fetchProgress = apt.progress.FetchProgress()
            return self._cache.Update(fetchProgress, self._list)
        finally:
            os.close(lock)
        
    def installArchives(self, pm, installProgress):
        installProgress.startUpdate()
        res = installProgress.run(pm)
        installProgress.finishUpdate()
        return res
    
    def commit(self, fetchProgress=None, installProgress=None):
        """ Apply the marked changes to the cache """
        # FIXME:
        # use the new acquire/pkgmanager interface here,
        # raise exceptions when a download or install fails
        # and send proper error strings to the application.
        # Current a failed download will just display "error"
        # which is less than optimal!

        if fetchProgress == None:
            fetchProgress = apt.progress.FetchProgress()
        if installProgress == None:
            installProgress = apt.progress.InstallProgress()

        pm = apt_pkg.GetPackageManager(self._depcache)
        fetcher = apt_pkg.GetAcquire(fetchProgress)
        while True:
            # fetch archives first
            res = self._fetchArchives(fetcher, pm)

            # then install
            res = self.installArchives(pm, installProgress)
            if res == pm.ResultCompleted:
                break
            if res == pm.ResultFailed:
                raise SystemError, "installArchives() failed"
            # reload the fetcher for media swaping
            fetcher.Shutdown()
        return (res == pm.ResultCompleted)

    # cache changes
    def cachePostChange(self):
        " called internally if the cache has changed, emit a signal then "
        self._runCallbacks("cache_post_change")

    def cachePreChange(self):
        """ called internally if the cache is about to change, emit
            a signal then """
        self._runCallbacks("cache_pre_change")

    def connect(self, name, callback):
        """ connect to a signal, currently only used for
            cache_{post,pre}_{changed,open} """
        if not self._callbacks.has_key(name):
            self._callbacks[name] = []
        self._callbacks[name].append(callback)

# ----------------------------- experimental interface
class Filter(object):
    """ Filter base class """
    def apply(self, pkg):
        """ Filter function, return True if the package matchs a
            filter criteria and False otherwise
        """
        return True

class MarkedChangesFilter(Filter):
    """ Filter that returns all marked changes """
    def apply(self, pkg):
        if pkg.markedInstall or pkg.markedDelete or pkg.markedUpgrade:
            return True
        else:
            return False

class FilteredCache(object):
    """ A package cache that is filtered.

        Can work on a existing cache or create a new one
    """
    def __init__(self, cache=None, progress=None):
        if cache == None:
            self.cache = Cache(progress)
        else:
            self.cache = cache
        self.cache.connect("cache_post_change", self.filterCachePostChange)
        self.cache.connect("cache_post_open", self.filterCachePostChange)
        self._filtered = {}
        self._filters = []
    def __len__(self):
        return len(self._filtered)
    
    def __getitem__(self, key):
        return self.cache._dict[key]

    def keys(self):
        return self._filtered.keys()

    def has_key(self, key):
        return self._filtered.has_key(key)

    def _reapplyFilter(self):
        " internal helper to refilter "
        self._filtered = {}
        for pkg in self.cache._dict.keys():
            for f in self._filters:
                if f.apply(self.cache._dict[pkg]):
                    self._filtered[pkg] = 1
                    break
    
    def setFilter(self, filter):
        " set the current active filter "
        self._filters = []
        self._filters.append(filter)
        #self._reapplyFilter()
        # force a cache-change event that will result in a refiltering
        self.cache.cachePostChange()

    def filterCachePostChange(self):
        " called internally if the cache changes, emit a signal then "
        #print "filterCachePostChange()"
        self._reapplyFilter()

#    def connect(self, name, callback):
#        self.cache.connect(name, callback)

    def __getattr__(self, key):
        " we try to look exactly like a real cache "
        #print "getattr: %s " % key
        if self.__dict__.has_key(key):
            return self.__dict__[key]
        else:
            return getattr(self.cache, key)
            

def cache_pre_changed():
    print "cache pre changed"

def cache_post_changed():
    print "cache post changed"


# internal test code
if __name__ == "__main__":
    print "Cache self test"
    apt_pkg.init()
    c = Cache(apt.progress.OpTextProgress())
    c.connect("cache_pre_change", cache_pre_changed)
    c.connect("cache_post_change", cache_post_changed)
    print c.has_key("aptitude")
    p = c["aptitude"]
    print p.name
    print len(c)

    for pkg in c.keys():
        x= c[pkg].name

    c.upgrade()
    changes = c.getChanges()
    print len(changes)
    for p in changes:
        #print p.name
        x = p.name


    # see if fetching works
    for d in ["/tmp/pytest", "/tmp/pytest/partial"]:
        if not os.path.exists(d):
            os.mkdir(d)
    apt_pkg.Config.Set("Dir::Cache::Archives","/tmp/pytest")
    pm = apt_pkg.GetPackageManager(c._depcache)
    fetcher = apt_pkg.GetAcquire(apt.progress.TextFetchProgress())
    c._fetchArchives(fetcher, pm)
    #sys.exit(1)

    print "Testing filtered cache (argument is old cache)"
    f = FilteredCache(c)
    f.cache.connect("cache_pre_change", cache_pre_changed)
    f.cache.connect("cache_post_change", cache_post_changed)
    f.cache.upgrade()
    f.setFilter(MarkedChangesFilter())
    print len(f)
    for pkg in f.keys():
        #print c[pkg].name
        x = f[pkg].name
    
    print len(f)

    print "Testing filtered cache (no argument)"
    f = FilteredCache(progress=OpTextProgress())
    f.cache.connect("cache_pre_change", cache_pre_changed)
    f.cache.connect("cache_post_change", cache_post_changed)
    f.cache.upgrade()
    f.setFilter(MarkedChangesFilter())
    print len(f)
    for pkg in f.keys():
        #print c[pkg].name
        x = f[pkg].name
    
    print len(f)
