from __future__ import print_function, unicode_literals
import os
import platform
import sys
from argparse import Namespace

from tools import skip_then_continue

cycdir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
reldir = os.path.join(cycdir, 'release')
blddir = os.path.join(cycdir, 'build')
sys.path.insert(0, reldir)

try:
    import smbchk
except ImportError:
    smbchk = False

def test_load():
    raise DeprecatedTest("symbol test has been deprecated")
    if not smbchk:
        return
    ns = Namespace(filename=os.path.join(reldir, 'symbols.json'))
    db = smbchk.load(ns)
    assert(isinstance(db, list))

def test_nm():
    raise DeprecatedTest("symbol test has been deprecated")
    if platform.system() == 'Darwin':
        skip_then_continue("Skipping for Mac")
    if not smbchk:
        return
    if os.name != 'posix' or not os.path.isdir(blddir):
        return
    ns = Namespace(prefix=blddir)
    syms = smbchk.nm(ns)
    assert ("cyclus::Agent::Agent(cyclus::Context*)" in  syms)

def test_diff():
    raise DeprecatedTest("symbol test has been deprecated")
    if not smbchk:
        return
    db = [{'symbols': ["cyclus::Agent::Agent(cyclus::Context*)"],
           'version': 'X', 'date': 'x.x.x'},
          {'symbols': ["cyclus::Agent::Agent(cyclus::Context*)",
                       "cyclus::Agent::~Agent()"],
           'version': 'Y', 'date': 'y.y.y'},]
    obs = smbchk.diff(db, 0, 1)
    assert(len(obs) > 0)

def test_check():
    raise DeprecatedTest("symbol test has been deprecated")
    if not smbchk:
        return
    # adds to API
    db = [{'symbols': ["cyclus::Agent::Agent(cyclus::Context*)"],
           'version': 'X', 'date': 'x.x.x'},
          {'symbols': ["cyclus::Agent::Agent(cyclus::Context*)",
                       "cyclus::Agent::~Agent()"],
           'version': 'Y', 'date': 'y.y.y'},]
    obs = smbchk.check(db)
    assert(obs)

    # removes from API
    db.append({'symbols': ["cyclus::Agent::~Agent()"],
               'version': 'Z', 'date': 'z.z.z'})
    obs = smbchk.check(db)
    assert not(obs)

if __name__ == "__main__":
    nose.runmodule()
