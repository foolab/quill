#! /bin/bash

debug=true
extraoptions=( )

for opt in "$@" ; do
	case ${opt} in
		debug=yes)
			debug=true
			;;
		debug=no)
			debug=false
			;;
		*)
			extraoptions=( "${extraoptions[@]}" "${opt}" )
			;;
	esac
done

options=( )
if ${debug} ; then
	options=( "${options[@]}" "CONFIG*=debug" "CONFIG-=release" )
else
	options=( "${options[@]}" "CONFIG-=debug" "CONFIG*=release" )
fi

qmake "${options[@]}" "${extraoptions[@]}"
