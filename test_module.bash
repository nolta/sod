#!/bin/bash
set -e -u -E
printf "$0: "
trap 'echo error at line $LINENO' ERR

source module.bash

[[ $(module --version) =~ ^sod\ [0-9]+\.[0-9]+\.[0-9]+$ ]] || false

# set/unset

unset XYZ
[ -z ${XYZ+x} ]
__sod_set XYZ 'x y z'
[ ! -z ${XYZ+x} ]
[ "$XYZ" == 'x y z' ]
__sod_unset XYZ
[ -z ${XYZ+x} ]

# push/pop

unset XYZ
__sod_push XYZ a
[ "$XYZ" == 'a' ]
__sod_push XYZ b
[ "$XYZ" == 'b:a' ]
__sod_push XYZ c
[ "$XYZ" == 'c:b:a' ]
__sod_push XYZ d
[ "$XYZ" == 'd:c:b:a' ]
__sod_push XYZ e
[ "$XYZ" == 'e:d:c:b:a' ]
__sod_pop XYZ e
[ "$XYZ" == 'd:c:b:a' ]
__sod_pop XYZ c
[ "$XYZ" == 'd:b:a' ]
__sod_pop XYZ a
[ "$XYZ" == 'd:b' ]
__sod_pop XYZ d
[ "$XYZ" == 'b' ]
__sod_pop XYZ b
[ -z ${XYZ+x} ]
__sod_push XYZ b
[ "$XYZ" == 'b' ]
__sod_push XYZ 'x y z'
[ "$XYZ" == 'x y z:b' ]
__sod_push XYZ 'u $v'
[ "$XYZ" == 'u $v:x y z:b' ]
__sod_pop XYZ 'x y z'
[ "$XYZ" == 'u $v:b' ]
__sod_push XYZ 'a:b'
[ "$XYZ" == 'a:b:u $v:b' ]
__sod_pop XYZ 'a:b'
[ "$XYZ" == 'u $v:b' ]
__sod_push XYZ 'a:b'
__sod_pop XYZ 'b'
[ "$XYZ" == 'a:u $v:b' ]
__sod_pop XYZ 'u $v'
[ "$XYZ" == 'a:b' ]
__sod_push XYZ ''
[ "$XYZ" == ':a:b' ]
__sod_pop XYZ a
[ "$XYZ" == ':b' ]
__sod_push XYZ 'c'
[ "$XYZ" == 'c::b' ]
__sod_pop XYZ ''
[ "$XYZ" == 'c:b' ]

echo OK
exit 0
