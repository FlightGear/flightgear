# Install bash and zsh completion files.
#
# The install path is not detected at runtime (contrary to what the
# bash-completion documentation suggests, see
# <https://github.com/scop/bash-completion/blob/master/README.md>) to account
# for those cases where the FlightGear installation prefix is not /usr.
# See <https://sourceforge.net/p/flightgear/flightgear/merge-requests/96/#e6f4>
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    install(FILES completion/fg-completion.bash DESTINATION share/bash-completion/completions RENAME fgfs)
    install(FILES completion/fg-completion.zsh DESTINATION share/zsh/site-functions RENAME _fgfs)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
