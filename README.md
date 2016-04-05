# ltetrigger
GNU Radio out-of-tree module for triggering on LTE signal

## Quickstart

* Install srsLTE
```bash
$ git clone https://github.com/srsLTE/srsLTE
$ # Fix a naming collision between srsLTE and log4cpp, a GNU Radio dependency:
$ cd srslte
$ grep -R -l " DEBUG(" . |xargs sed -i 's/ DEBUG(/ SRSLTE_DEBUG(/g'
$ # One little stinker got away... get it:
$ find -name ue_cell_search.c |xargs sed -i 's/DEBUG/SRSLTE_DEBUG/g'
$ mkdir build; cd build
$ cmake ../
$ make && make test
$ make install
```

* Install LTETrigger
```bash
$ git clone https://github.com/NTIA/ltetrigger
$ cd ltetrigger/gr-ltetrigger
$ mkdir build; cd build
$ cmake ../
$ make && make test
$ sudo make install
$ sudo ldconig # This may be necessary, definitely won't hurt
```

* Run the LTETrigger example
```bash
$ cd ../../examples
$ ./cell_search_file.py -s 1.92M -f 2145M --repeat -c 19.2M ../gr-ltetrigger/python/lte_test_frames
Using Volk machine: avx2_64_mmx
{'nports': 1L, 'linktype': 'downlink', 'nprb': 6L, 'cell_id': 369L}
```
