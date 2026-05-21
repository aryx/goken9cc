# Environment setup for the goken9cc Plan 9 / kencc toolchain.
# Sourced by login shells via /etc/profile.
#
# We intentionally do NOT export GOROOT here: the goken binaries are built
# with /usr/lib/goken9cc baked in as the default install prefix, and
# GOROOT is also used by the Go toolchain (`golang`). Exporting it from
# /etc/profile.d would silently break Go.

case "$(uname -m)" in
    x86_64)        _goken_arch=amd64 ;;
    aarch64|arm64) _goken_arch=arm64 ;;
    *)             _goken_arch= ;;
esac

if [ -n "$_goken_arch" ] && [ -d "/usr/lib/goken9cc/$_goken_arch/bin" ]; then
    # Append (not prepend) so Plan 9 utilities like cat/ls/grep don't
    # shadow the system ones; toolchain binaries (5c/6c/8l/mk/rc/...) are
    # uniquely named and remain reachable.
    case ":$PATH:" in
        *":/usr/lib/goken9cc/$_goken_arch/bin:"*) ;;
        *) PATH="$PATH:/usr/lib/goken9cc/$_goken_arch/bin" ;;
    esac
    export PATH

    # mk hardcodes /bin/rc as the default shell; point it at our rc.
    : "${MKSHELL:=/usr/lib/goken9cc/$_goken_arch/bin/rc}"
    export MKSHELL

    # cc (6c/8c/...) looks at /sys/include and /<arch>/include by default
    # (Plan 9 paths that don't exist on Linux). Point it at our headers.
    : "${INCLUDE:=/usr/lib/goken9cc/include}"
    export INCLUDE

    # 8l/7l/5l/vl resolve -l<name> against $ccroot/<arch>/lib (defaulting
    # to /<arch>/lib if unset). Point ccroot at our install prefix.
    : "${ccroot:=/usr/lib/goken9cc}"
    export ccroot
fi

unset _goken_arch
