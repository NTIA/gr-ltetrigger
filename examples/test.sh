set -o verbose

./cell_search_file.py --sample-rate 1.92M ../test_frames/lte_frame_6prb_cellid_123 --repeat --time-out 1
./cell_search_file.py --sample-rate 7.68M ../test_frames/lte_frame_25prb_cellid_124 --repeat --time-out 1
./cell_search_file.py --sample-rate 15.36M ../test_frames/lte_frame_50prb_cellid_125 --repeat --time-out 1
./cell_search_file.py --sample-rate 30.72M ../test_frames/lte_frame_100prb_cellid_369 --repeat --time-out 1
