
# usage
```bash
# after cloning repository - setup submodules
git submodule init
git submodule update

# build & run
DIR_BUILD=.build \
    && cmake -B $DIR_BUILD \
    && make -j8 -C $DIR_BUILD \
    && $DIR_BUILD/ced
```
