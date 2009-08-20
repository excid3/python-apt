# import the core of apt_pkg
import apt_pkg
import sys
import os

# import some fancy classes
from apt.package import Package
from apt.cache import Cache
from apt.progress import OpProgress, FetchProgress, InstallProgress, CdromProgress
from apt.cdrom import Cdrom
from apt_pkg import SizeToStr, TimeToStr, VersionCompare

# init the package system
apt_pkg.init()


import warnings
warnings.warn("apt API not stable yet", FutureWarning)
del warnings
