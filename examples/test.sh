set -o verbose

./cell_search_file.py --sample-rate 1.92M ../python/test_data/lte_frame_6prb_cellid_123 --repeat --time-out 1
./cell_search_file.py --sample-rate 7.68M ../python/test_data/lte_frame_25prb_cellid_124 --repeat --time-out 1
./cell_search_file.py --sample-rate 15.36M ../python/test_data/lte_frame_50prb_cellid_125 --repeat --time-out 1
./cell_search_file.py --sample-rate 30.72M ../python/test_data/lte_frame_100prb_cellid_369 --repeat --time-out 1
