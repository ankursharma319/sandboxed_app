#! /bin/sh

# run ./configure.sh first

# also make sure the cgroup exists
#
# sudo mkdir /sys/fs/cgroup/my_sandbox
# echo "+memory" | sudo tee /sys/fs/cgroup/my_sandbox/cgroup.subtree_control
# sudo mkdir /sys/fs/cgroup/my_sandbox/leaf
#
# sudo chown -R ubuntu:ubuntu /sys/fs/cgroup/my_sandbox

# Only leaf cgroups can have member processes and should not have anything
# in cgroup.subtree_control file. Obviously, the parents cgroup.subtree_control
# will not be empty and contain the needed memory controller.
# https://unix.stackexchange.com/questions/680167/ebusy-when-trying-to-add-process-to-cgroup-v2

cmake --build _build/release/ \
&& sudo SANDBOX_CGROUP_DIR="/sys/fs/cgroup/my_sandbox/leaf" ./_build/release/sandboxed_executable

#cmake --install _build/ --prefix _install/
#_install/bin/MainExe

