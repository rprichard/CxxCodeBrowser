# =============================================================================
# HACK: Work around a bug in qmake's Makefile generator.  A link_url pro file
# is qmake'd before qmake has generated the dependencies' prl files.  The
# problem probably only happens with parallel make.
#
# Note: The qmake Makefile generator uses recursive make.  Each pro file is
# turned into a single standalone Makefile.  The toplevel subdirs Makefile
# reinvokes qmake to generate each subdir Makefile.
#
# There are two ways of creating a dependency between two SUBDIRs.  The
# documented way is to add "CONFIG += ordered" to the subdirs pro file.
# Another way, undocumented, is to add a variable to the SUBDIRS setting and
# set the variable's depends setting[1].  In either case, the Makefile
# corresponding to the subdirs pro file will force the dependency to be make'd
# before the dependent is make'd.  The problem is that the toplevel Makefile
# allows running qmake to generate the subdir Makefiles in any order.  When a
# SUBDIR uses create_prl to export its indirect dependencies, the prl file is
# generated when the toplevel Makefile calls qmake on the subdir.  Therefore,
# it is possible for the toplevel Makefile to call qmake on the link_prl
# dependent pro file first, before the dependency's prl file exists.  This
# quietly loses the indirect dependencies, leading to unresolved symbol linker
# errors.
#
# The QtCreator IDE dodges the problem by passing -r to qmake, which makes
# qmake recursively generate all the Makefiles and prl files at once, in
# top-to-bottom, depth-first order.  I could do the same, but I have no
# reasonable way to force users to pass -r to qmake, and the consequences of
# omitting -r (unpredictable linker errors) are too severe.  Some Symbian
# mkspec files use an undocumented "option(recursive)" syntax, which seems to
# force -r on, but then QtCreator complains about it[2] every time it parses
# the pro file.  The Psi project also ran into the same problem, but decided to
# require the use of -r[3].
#
# The workaround used here is to create a FOO-qmake subdir for every FOO subdir
# that uses link_prl.  The FOO-qmake subdir is itself a subdirs project that
# includes ../FOO.  With this approach, FOO/Makefile will not be generated
# until after "make FOO-qmake/Makefile" is run, which does not happen until the
# dependencies are make'd.
#
# [1] http://www.qtcentre.org/wiki/index.php?title=Undocumented_qmake#SUBDIRS_projects
# [2] Foo.pro(123): Unknown option() recursive.
# [3] http://comments.gmane.org/gmane.network.jabber.psi.devel/8227
# =============================================================================

TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    third_party \
    libbtrace \
    libindexdb \
    clang-indexer-qmake \
    index-tool-qmake \
    navigator-qmake

btrace_script.path = /
btrace_script.files += libbtrace/btrace.sh
INSTALLS += btrace_script

# HACK: Stop qmake from attempting to strip btrace.sh.
QMAKE_STRIP = /bin/true
