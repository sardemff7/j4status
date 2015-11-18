m4_ifndef([PKG_INSTALLDIR], [AC_DEFUN([PKG_INSTALLDIR], [
    pkgconfigdir='${libdir}'/pkgconfig
    AC_SUBST([pkgconfigdir])
])])

m4_ifndef([PKG_NOARCH_INSTALLDIR], [AC_DEFUN([PKG_NOARCH_INSTALLDIR], [
    noarch_pkgconfigdir='${datadir}'/pkgconfig
    AC_SUBST([noarch_pkgconfigdir])
])])
