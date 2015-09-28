AC_DEFUN([J4SP_INIT], [
    m4_ifnblank([$1], [
        PKG_CHECK_MODULES(m4_default([$2], [J4STATUS_PLUGIN]), [libj4status-plugin $3])
        AC_ARG_WITH([$1], AS_HELP_STRING([--with-][$1][=DIR], [Directory for j4status plugins]), [], [with_][$1][=yes])
        case "$with_[$1]" in
            no|"") AC_MSG_ERROR([*** You must define $1]) ;;
            yes) [$1]="`$PKG_CONFIG --variable=pluginsdir libj4status-plugin`" ;;
            *) [$1]="$with_[$1]" ;;
        esac
        AC_SUBST([$1])
    ])
])

AC_DEFUN([_J4SP_PRINT_PLUGIN_SUMMARY_INTERNAL], [
    _j4sp_plugin_status="$2"
    AS_CASE([$1][$2],
        [yesyes], [_j4sp_plugin_status="$_j4sp_plugin_status (Default, disable with --disable-$3)"],
        [yesno], [_j4sp_plugin_status="$_j4sp_plugin_status (Disabled)"],
        [nono], [_j4sp_plugin_status="$_j4sp_plugin_status (Default, enable with --enable-$3)"],
        [noyes], [_j4sp_plugin_status="$_j4sp_plugin_status (Enabled)"]
    )
    AC_MSG_RESULT([        ]m4_map_args_sep([], [], [ ], m4_shift3($@))[: $_j4sp_plugin_status])
])

AC_DEFUN([_J4SP_PRINT_PLUGIN_SUMMARY], [
    _J4SP_PRINT_PLUGIN_SUMMARY_INTERNAL(m4_unquote(m4_split([$1])))
])

AC_DEFUN([J4SP_SUMMARY], [
    m4_ifnblank([_J4SP_OUTPUT_PLUGINS][_J4SP_INPUT_OUTPUT_PLUGINS], [
        AC_MSG_RESULT([    Output plugins:])
        m4_map([_J4SP_PRINT_PLUGIN_SUMMARY], [_J4SP_OUTPUT_PLUGINS])
        m4_map([_J4SP_PRINT_PLUGIN_SUMMARY], [_J4SP_INPUT_OUTPUT_PLUGINS])
    ])
    m4_ifnblank([_J4SP_INPUT_PLUGINS][_J4SP_INPUT_OUTPUT_PLUGINS], [
        AC_MSG_RESULT([    Input plugins:])
        m4_map([_J4SP_PRINT_PLUGIN_SUMMARY], [_J4SP_INPUT_PLUGINS])
        m4_map([_J4SP_PRINT_PLUGIN_SUMMARY], [_J4SP_INPUT_OUTPUT_PLUGINS])
    ])
])

AC_DEFUN([_J4SP_ADD_PLUGIN], [
    AC_REQUIRE([J4SP_INIT])

    m4_define([_j4sp_plugin], m4_translit(m4_tolower([$2]), [ _], [--]))
    m4_define([_j4sp_enable_arg], _j4sp_plugin[-$1])
    m4_define([_j4sp_enable_var], [enable_]m4_translit(_j4sp_enable_arg, [-], [_]))

    m4_case([$4],
    [always], _j4sp_enable_var[=yes],
    [yes],    [AC_ARG_ENABLE(_j4sp_enable_arg, AS_HELP_STRING([--disable-]_j4sp_enable_arg, [Disable $3 plugin]), [], _j4sp_enable_var[=yes])],
              [AC_ARG_ENABLE(_j4sp_enable_arg, AS_HELP_STRING([--enable-]_j4sp_enable_arg, [Enable $3 plugin]), [], _j4sp_enable_var[=no])]
    )
    AM_CONDITIONAL(m4_toupper(_j4sp_enable_var), [test "x$]_j4sp_enable_var[" = xyes])

    AS_IF([test "x$]_j4sp_enable_var[" = xyes], [
        $5
        [AM_DOCBOOK_CONDITIONS="${AM_DOCBOOK_CONDITIONS};]_j4sp_enable_var["]
    ])

    m4_append([_J4SP_]m4_translit(m4_toupper([$1]), [-], [_])[_PLUGINS], [$4 $]_j4sp_enable_var[ ]_j4sp_enable_arg[ $3], [,])

    m4_undefine([_j4sp_enable_var])
    m4_undefine([_j4sp_enable_arg])
    m4_undefine([_j4sp_plugin])
])

AC_DEFUN([J4SP_ADD_INPUT_OUTPUT_PLUGIN], [
    _J4SP_ADD_PLUGIN([input-output], [$1], [$2], [$3], [$4])
])

AC_DEFUN([J4SP_ADD_INPUT_PLUGIN], [
    _J4SP_ADD_PLUGIN([input], [$1], [$2], [$3], [$4])
])

AC_DEFUN([J4SP_ADD_OUTPUT_PLUGIN], [
    _J4SP_ADD_PLUGIN([output], [$1], [$2], [$3], [$4])
])
