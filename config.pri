# The install directories can be configured in two ways:
#
#  * At qmake/configure-time, by setting PREFIX.  This is the conventional
#    method of configuring Unix software.  The configure script defaults to
#    invoking qmake with a PREFIX of /usr/local.
#
#  * At "make install"-time, by setting the INSTALL_ROOT environment/make
#    variable.  The INSTALL_ROOT variable is built-in to qmake and is the way
#    that qmake was designed to configure the install directory.

equals(PREFIX, "") {
    PREFIX = /
}
BIN_DIR = $${PREFIX}/bin
BIN_DIR ~= s:/+:/
LIBEXEC_DIR = $${PREFIX}/libexec
LIBEXEC_DIR ~= s:/+:/
