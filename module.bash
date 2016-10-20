
function __sod_push() {
    export "$1=$2${!1:+:}${!1-}"
}

function __sod_pop() {
    local IFS=':'
    local a=(${!1})
    for i in ${!a[@]}; do
        if [[ "${a[$i]}" == "$2" ]]; then
            unset a[$i]
            break
        fi
    done
    export "$1=${a[*]-}"
}

function __sod_set() {
    export "$1=$2"
}

function __sod_unset() {
    unset $1
}

function module() {
    while read line; do
        case "$line" in
            'echo '*) echo "${line:5}" ;;
            *) eval "$line" ;;
        esac
    done < <(sod "$@")
}

# tab completion

function _module_tabcomplete() {
    local cur cmds
    cur="${COMP_WORDS[COMP_CWORD]}"
    cmds="avail list load unload use purge search"

    COMPREPLY=()
    case $COMP_CWORD in
    0|1)
        COMPREPLY=( $(compgen -W "$cmds" -- $cur) )
        ;;
    *)
        case ${COMP_WORDS[1]} in
        load)
            COMPREPLY=( $(compgen -W "$(sod avail | cut -d' ' -f2)" -- $cur) )
            ;;
        unload)
            COMPREPLY=( $(IFS=: compgen -W "$__sod_installed" -- $cur) )
            ;;
        esac
    esac
}

complete -F _module_tabcomplete module
