#! /bin/bash

# cmake --log-level=VERBOSE -S ./ -B ./_build/debug/ -D CMAKE_BUILD_TYPE=Debug
cmake --log-level=VERBOSE -S ./ -B ./_build/release/ -D CMAKE_BUILD_TYPE=Release
# if [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
#     cmake --log-level=VERBOSE -S ./ -B ./_build/gprof_ready/ -D CMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-pg -DCMAKE_EXE_LINKER_FLAGS=-pg -DCMAKE_SHARED_LINKER_FLAGS=-pg
# fi

#For clangd
cmake --log-level=VERBOSE -S ./ -B ./_build/compile_commands_json/ -D CMAKE_EXPORT_COMPILE_COMMANDS=1
link_src=$PWD/_build/compile_commands_json/compile_commands.json
link_target=$PWD/compile_commands.json
if [[ -L ${link_target} ]] && [[ -e ${link_target} ]]
then
    :
else
    echo "creating symlink"
    ln -s $link_src $link_target
fi
