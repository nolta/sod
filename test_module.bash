#!/bin/bash
set -e -u -E
trap 'echo error at line $LINENO' ERR
trap '[[ $? == 0 ]] && echo OK' EXIT

source module.bash

# set/unset

unset XYZ
[[ ! -v XYZ ]]
__sod_set XYZ 'x y z'
[[ -v XYZ ]]
[[ "$XYZ" == 'x y z' ]]
__sod_unset XYZ
[[ ! -v XYZ ]]

# push/pop

unset XYZ
__sod_push XYZ a
[[ "$XYZ" == 'a' ]]
__sod_push XYZ b
[[ "$XYZ" == 'b:a' ]]
__sod_push XYZ c
[[ "$XYZ" == 'c:b:a' ]]
__sod_push XYZ d
[[ "$XYZ" == 'd:c:b:a' ]]
__sod_push XYZ e
[[ "$XYZ" == 'e:d:c:b:a' ]]
__sod_pop XYZ e
[[ "$XYZ" == 'd:c:b:a' ]]
__sod_pop XYZ c
[[ "$XYZ" == 'd:b:a' ]]
__sod_pop XYZ a
[[ "$XYZ" == 'd:b' ]]
__sod_pop XYZ d
[[ "$XYZ" == 'b' ]]
__sod_pop XYZ b
[[ "$XYZ" == '' ]]
__sod_push XYZ b
[[ "$XYZ" == 'b' ]]
__sod_push XYZ 'x y z'
[[ "$XYZ" == 'x y z:b' ]]
__sod_push XYZ 'u $v'
[[ "$XYZ" == 'u $v:x y z:b' ]]
__sod_pop XYZ 'x y z'
[[ "$XYZ" == 'u $v:b' ]]
__sod_push XYZ 'a:b'
[[ "$XYZ" == 'a:b:u $v:b' ]]
__sod_pop XYZ 'b'
[[ "$XYZ" == 'a:u $v:b' ]]
__sod_pop XYZ 'u $v'
[[ "$XYZ" == 'a:b' ]]
__sod_push XYZ ''
[[ "$XYZ" == ':a:b' ]]
__sod_pop XYZ a
[[ "$XYZ" == ':b' ]]
__sod_push XYZ 'c'
[[ "$XYZ" == 'c::b' ]]
__sod_pop XYZ ''
[[ "$XYZ" == 'c:b' ]]

exit 0