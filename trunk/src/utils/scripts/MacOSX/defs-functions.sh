
RED="\033[1;31m"
BLUE="\033[0;34m"
GREEN="\033[0;36m"
NONE="\033[0m"

function pn
{
	echo -e $@
}

function pc
{
	echo -en $1
	shift
	pn $@
	echo -en $NONE
}

