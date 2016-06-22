
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

