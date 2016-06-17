
function __sod_push() {
    if [[ -v $1 ]]; then
        local IFS=':'
        local a=("$2" ${!1})
        export "$1=${a[*]}"
    else
        export "$1=$2"
    fi
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
    source <(sod "$@")
}

