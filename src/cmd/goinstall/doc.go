// Copyright 2010 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

/*

Goinstall is an experiment in automatic package installation.
It installs packages, possibly downloading them from the internet.
It maintains a list of public Go packages at http://godashboard.appspot.com/package.

Usage:
	goinstall [flags] importpath...
	goinstall [flags] -a

Flags and default settings:
        -a=false          install all previously installed packages
	-dashboard=true   tally public packages on godashboard.appspot.com
	-log=true         log installed packages to $GOROOT/goinstall.log for use by -a
	-u=false          update already-downloaded packages
	-v=false          verbose operation

Goinstall installs each of the packages identified on the command line.  It
installs a package's prerequisites before trying to install the package
itself. Unless -log=false is specified, goinstall logs the import path of each
installed package to $GOROOT/goinstall.log for use by goinstall -a.

If the -a flag is given, goinstall reinstalls all previously installed
packages, reading the list from $GOROOT/goinstall.log.  After updating to a
new Go release, which deletes all package binaries, running

	goinstall -a

will recompile and reinstall goinstalled packages.

Another common idiom is to use

	goinstall -a -u

to update, recompile, and reinstall all goinstalled packages.

The source code for a package with import path foo/bar is expected
to be in the directory $GOROOT/src/pkg/foo/bar/.  If the import
path refers to a code hosting site, goinstall will download the code
if necessary.  The recognized code hosting sites are:

	BitBucket (Mercurial)

		import "bitbucket.org/user/project"
		import "bitbucket.org/user/project/sub/directory"

	GitHub (Git)

		import "github.com/user/project.git"
		import "github.com/user/project.git/sub/directory"

	Google Code Project Hosting (Mercurial, Subversion)

		import "project.googlecode.com/hg"
		import "project.googlecode.com/hg/sub/directory"

		import "project.googlecode.com/svn/trunk"
		import "project.googlecode.com/svn/trunk/sub/directory"

	Launchpad

		import "launchpad.net/project"
		import "launchpad.net/project/series"
		import "launchpad.net/project/series/sub/directory"

		import "launchpad.net/~user/project/branch"
		import "launchpad.net/~user/project/branch/sub/directory"


If the destination directory (e.g., $GOROOT/src/pkg/bitbucket.org/user/project)
already exists and contains an appropriate checkout, goinstall will not
attempt to fetch updates.  The -u flag changes this behavior,
causing goinstall to update all remote packages encountered during
the installation.

When downloading or updating, goinstall first looks for a tag or branch
named "release".  If there is one, it uses that version of the code.
Otherwise it uses the default version selected by the version control
system, typically HEAD for git, tip for Mercurial.

After a successful download and installation of a publicly accessible
remote package, goinstall reports the installation to godashboard.appspot.com,
which increments a count associated with the package and the time
of its most recent installation.  This mechanism powers the package list
at http://godashboard.appspot.com/package, allowing Go programmers
to learn about popular packages that might be worth looking at.
The -dashboard=false flag disables this reporting.

By default, goinstall prints output only when it encounters an error.
The -v flag causes goinstall to print information about packages
being considered and installed.

Goinstall does not attempt to be a replacement for make.
Instead, it invokes "make install" after locating the package sources.
For local packages without a Makefile and all remote packages,
goinstall creates and uses a temporary Makefile constructed from
the import path and the list of Go files in the package.
*/
package documentation
