
# usage
```bash
# after cloning repository - setup submodules
git submodule init
git submodule update

# install important dev libraries
sudo apt install libwayland-dev libxkbcommon-dev xorg-dev # on ubuntusway

# build & run
DIR_BUILD=.build \
    && cmake -B $DIR_BUILD \
    && make -j8 -C $DIR_BUILD \
    && $DIR_BUILD/ced
```
