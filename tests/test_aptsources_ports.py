#!/usr/bin/env python

import unittest
import apt_pkg
import os
import copy

import sys
sys.path.insert(0, "../")
import aptsources
import aptsources.sourceslist
import aptsources.distro

class TestAptSources(unittest.TestCase):
    def __init__(self, methodName):
        unittest.TestCase.__init__(self, methodName)
        apt_pkg.init()
        apt_pkg.Config.Set("APT::Architecture","powerpc")
        apt_pkg.Config.Set("Dir::Etc", os.path.join(os.getcwd(),"test-data-ports"))
        apt_pkg.Config.Set("Dir::Etc::sourceparts","/xxx")

    def testMatcher(self):
        apt_pkg.Config.Set("Dir::Etc::sourcelist","sources.list")
        sources = aptsources.sourceslist.SourcesList()
        distro = aptsources.distro.get_distro("Ubuntu","hardy","desc","8.04")
        distro.get_sources(sources)
        # test if all suits of the current distro were detected correctly
        dist_templates = set()
        for s in sources:
            if not s.line.strip() or s.line.startswith("#"):
                continue
            if not s.template:
                self.fail("source entry '%s' has no matcher" % s)


if __name__ == "__main__":
    unittest.main()
