
# build & run
## ubuntusway
```bash
DIR_BUILD=.build \
    && cmake -B $DIR_BUILD \
    && make -j8 -C $DIR_BUILD \
    && $DIR_BUILD/ced
```
