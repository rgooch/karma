#  .cshrc  shell script. This shell script should be sourced by
# every  Karma  user.


# Written by		Richard Gooch	22-SEP-1992

# Last updated by	Richard Gooch	15-APR-1993


# Alias definitions

# Miscellaneuos aliases
alias 'klint'	'lint -DARCH_$MACHINE_ARCH -I$KARMABASE/include -DVERSION=\"date_string\"'
alias 'kxlint'	'lint -DARCH_$MACHINE_ARCH -I$KARMABASE/include -I$XINCLUDEPATH -DVERSION=\"date_string\"'
alias 'push_karma'	"clean_karma > /dev/null ; backup -ralwsop /home/rgooch/karma /applic/karma"
alias 'clean_karma'	"find ~rgooch/karma \( -name '*~' -o -name '.*~' \) -print -exec rm {} \;"

if ("$?KARMAROOT" != "0") then
    # Set cd path
    if ("$?cdpath" == "0") then
	set cdpath = ($KARMAROOT)
    else
	set cdpath = ($KARMAROOT $cdpath)
    endif
endif
