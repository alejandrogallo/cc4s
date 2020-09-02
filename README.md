Coupled Cluster for Solids
==========================

Library dependencies
--------------------

- go to the lib subdirectory and execute:
  ```
  make [CONFIG=<config>] [Options]
  ```
- this should automatically fetch the submodules' source from the
  respective maintainers, configure the modules for the given configuration
  (See below) and build the modules for that configuration.
- by default the configuration `gxx` is used.

Building
--------

-   check `gxx` version. Need at least `gcc 4.7.2` (As tested, gcc 4.6 does
    not work).
-   write or edit a `config.<config>` file for your build environment.
-   the configuration files `config.gxx` and `config.icc` contain
    predifined retail environments for `gxx` and `icc`, respectively,
    using full optimization and without debugging info.
-   for the gxx configuration the following additional libraries are
    required
    - OpenBLAS
    - scalapack
- make sure, the above library dependencies are built for your configuration
- run `make -j 8 [CONFIG=<config]` to build for the desired environment, by
  default for `gxx`. The `-j` option issues a parallel make on 8 processes.
- run `make install [CONFIG=<config>]` to copy the executable to the specified
  target directory. The default is `~/bin/cc4s/<config>`.
- the intermediate build files for each build environment can be found in the
  directory `build/<config>/`.

Running
-------

-   a `cc4s` operation file, e.g. `mp2.yaml`, can be run with
    ```
    ~/bin/cc4s/gxx/Cc4s -i mp2.yaml
    ```

Testing
-------

### Running tests

- run the test suite in the test directory:
  ```
  ./test.sh [-c <config>] [-t <type>] [...]
  ```
  You can run
  ```
  ./test.sh -h
  ```
  to see all the available options.
- this issues all tests of the given type for local build binary of the given
  build environment.
- by default the `essential` tests are conducted for the `icc` build
  binary.
- note that installed executables are not tested unless you explicitly specify
  their location, e.g. by:
  ```
  ./test.sh -x ~/bin/cc4s/gxx/Cc4s
  ```

### Writing tests

-   To write a test you need to write a bash script with the name
    `doTest.sh`
  it is in the folder `test`.
- To define different groups of tests a CLASS attribute is provided.
  All tests are by default in the class `all`, and all important tests should
  be in the class `essential`. The class of the test is defined by writing
  in the header of the test file the line:
  ```
  # @CLASS=essential,example_class1,....
  ```
- A convention to follow is that the essential corresponds to the test file
  in the main parent directory of every test case. Subdirectories can
  allways be created containing also test files, which should typically
  marked as non-essential.
- To every test script global variables are made available by the test suite
  script `test.sh`. These are `RUN_COMMAN` and `CC4S_PATH`.  `RUN_COMMAND` is
  meant to be either empty or a form of mpi runner (e.g. mpirun).  `CC4S_PATH`
  is the path of the cc4s executable. You can run the test
  calculation by writing for example in the test script:
  ```
  ${RUN_COMMAND} ${CC4S_PATH} -file test.cc4s
  ```
- The main task of the test script is to set a variable upon termination to communicate
  if the test was succesful or not. This varible is `TEST_RESULT`. A value of 0 means
  that the test was succesful. Otherwise the test suite will consider the
  test as failed.    You can write other programs or scripts in order to
  decide if the test was successful or not, but the result must be
  communicated by setting these variables in the doTest.sh script.
- In order to print debug information during the implementation of the test,
  you can write to stderr since stdout is being overriden by the test suite
  runner. You can do this like so
  echo `Debug message` >&2

Update to newer module versions
-------------------------------

If any of the submodules are updated by their respective maintainer, you
can incorporate the latest version into cc4s. Note that this may lead to
incompatabilities and it must therefore be done with good care.

- if you intent to update the master, create a branch from the master.
  In case anything goes wrong the damage is controlled
- update the modules at the top level of the cc4s directory structure:
  ```
  git submodule foreach git pull origin master
  ```
- build the updated modules for all configurations supported as described above.
- build `cc4s` for all configurations supported
- run the `cc4s` `essental` tests for all configurations supported
- fix all bugs that emerged from advancing to the new version in `cc4s` or
  let them be fixed in the modules.

If all tests pass, `cc4s` may be advanced to the new modules\' versions
by
```
git commit -m "Updated submodules to latest version"
```

- if you want to advance the master branch, merge your branch into it.

Note that you may commit changes to the your branch even if things do
not work. However, each commit will be visible in the history.
