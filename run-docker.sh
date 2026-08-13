#!/bin/sh
# Run a command inside the NextUI cross-compile toolchain Docker container.
#
# The Music Player Makefile references sibling directories (../../all,
# ../../tg5040/platform, ../../$(PLATFORM)/libmsettings, ...), so the whole
# NextUI workspace must be visible inside the container, not just this repo.
# Rather than requiring this repo to be physically nested inside that
# workspace (which breaks for git worktrees -- the worktree lives elsewhere
# on disk), a `.nextui-workspace` symlink at the repo root points at the real
# NextUI workspace directory. That target is mounted at /root/workspace, and
# this repo -- wherever it actually lives -- is bind-mounted over the
# nextui-music-player/ slot inside it, so whatever checkout/worktree you're
# running from is what actually gets built.
#
# PLATFORM (default tg5040) selects the toolchain image. Usage:
#   sh run-docker.sh /bin/sh -c 'cd nextui-music-player/src && make PLATFORM=tg5040'
#   PLATFORM=tg5050 sh run-docker.sh /bin/sh -c 'cd nextui-music-player/src && make PLATFORM=tg5050'
set -e
PLATFORM=${PLATFORM:-tg5040}
HERE=$(cd "$(dirname "$0")" && pwd -P)
LINK="$HERE/.nextui-workspace"

if [ ! -e "$LINK" ]; then
    if [ -L "$LINK" ]; then
        echo "ERROR: $LINK is a broken symlink (target does not exist)." >&2
    else
        echo "ERROR: $LINK is missing." >&2
        echo "Create it pointing at your NextUI workspace directory, e.g.:" >&2
        echo "  ln -s /path/to/NextUI/workspace \"$LINK\"" >&2
    fi
    exit 1
fi

WORKSPACE=$(cd "$LINK" 2>/dev/null && pwd -P) || {
    echo "ERROR: $LINK exists but is not a directory (or is unreadable)." >&2
    exit 1
}

if [ ! -d "$WORKSPACE/all/common" ]; then
    echo "ERROR: $WORKSPACE doesn't look like a NextUI workspace (missing all/common)." >&2
    exit 1
fi

exec docker run -it --rm \
    -v "$WORKSPACE":/root/workspace \
    -v "$HERE":/root/workspace/nextui-music-player \
    "ghcr.io/loveretro/${PLATFORM}-toolchain" "$@"
