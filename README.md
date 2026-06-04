
``` sh
function ffpanel() {
    local CMD_FILE
    CMD_FILE="$(mktemp)" || return 1

    # always clean up the temp file on exit
    trap 'rm -f "$CMD_FILE"' RETURN

    "$HOME/.local/bin/ffpanel" "$@" > "$CMD_FILE" || return $?

    # show the command before running (like a built-in dry-run preview)
    echo "Running: $(cat "$CMD_FILE")"

    . "$CMD_FILE"
}

```
