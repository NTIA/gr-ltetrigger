/* -*- c++ -*- */
/*
 * Copyright 2016 <+YOU OR YOUR COMPANY+>.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstdlib> /* calloc */
#include <cstring> /* memset */
#include <iostream> /* cout, cerr */

#include <gnuradio/io_signature.h>
#include <pmt/pmt.h>

#include <srslte/utils/vector.h>

#include "ltetrigger_c_impl.h"


#define TRACK_FRAME_SIZE        32
#define FIND_NOF_AVG_FRAMES     2
#define MAX_TIME_OFFSET         128

// TODO: move this
cf_t dummy[MAX_TIME_OFFSET];
#define TRACK_MAX_LOST          4
#define TRACK_FRAME_SIZE        32
#define FIND_NOF_AVG_FRAMES     2



namespace gr {
  namespace ltetrigger {

    // Initialize static variables
    uint32_t ltetrigger_c_impl::d_N_id_2 = 0; // 0, 1, or 2
    uint32_t ltetrigger_c_impl::d_nof_detected_frames = 0;
    uint32_t ltetrigger_c_impl::d_nof_scanned_frames = 0;
    uint32_t ltetrigger_c_impl::d_nof_cells_found = 0;

    // Set initialize state
    ltetrigger_c_impl::State ltetrigger_c_impl::state = ST_CELL_SEARCH_AND_SYNC;

    ltetrigger_c::sptr
    ltetrigger_c::make()
    {
      return gnuradio::get_initial_sptr(new ltetrigger_c_impl());
    }

    /*
     * The private constructor
     */
    ltetrigger_c_impl::ltetrigger_c_impl()
      : gr::sync_block("ltetrigger_c",
                       gr::io_signature::make(1, 1, sizeof(gr_complex)),
                       gr::io_signature::make(0, 0, 0))
    {
      // Set debug
      //srslte_verbose += 2;
      //assert(SRSLTE_VERBOSE_ISDEBUG());

      // srsLTE initialization below this line:

      std::memset(&cs, 0, sizeof(srslte_ue_cellsearch_t));
      cs.max_frames = SRSLTE_CS_DEFAULT_MAXFRAMES_TOTAL;
      cs.nof_frames_to_scan = SRSLTE_CS_DEFAULT_NOFFRAMES_TOTAL;
      cs.detect_threshold = 1.0;

      std::memset(found_cells, 0, 3*sizeof(srslte_ue_cellsearch_result_t));

      std::memset(&cell, 0, sizeof(srslte_cell_t));
      cell.id = SRSLTE_CELL_ID_UNKNOWN;
      cell.nof_prb = SRSLTE_CS_NOF_PRB;

      cs.candidates = static_cast<srslte_ue_cellsearch_result_t *>(
        calloc(cs.max_frames, sizeof(srslte_ue_cellsearch_result_t))
        );
      cs.mode_ntimes = static_cast<uint32_t *>(
        calloc(cs.max_frames, sizeof(uint32_t))
        );
      cs.mode_counted = static_cast<uint8_t *>(
        calloc(cs.max_frames, sizeof(uint8_t))
        );

      if (this->ue_sync_init(&cs.ue_sync, cell)) {
        std::cerr << "Error initiating ue_sync" << std::endl;
        exit(-1);
      }

      /* SRSLTE Note: We align memory to 32 bytes (for AVX compatibility)
       * because in some cases volk can incorrectly detect the architecture.
       * This could be inefficient for SSE or non-SIMD platforms but shouldn't
       * be a huge problem.
       */
      input_buffer = static_cast<cf_t *>(
        srslte_vec_malloc(2*cs.ue_sync.frame_len * sizeof(cf_t))
        );
      if (!input_buffer) {
        std::cerr << "malloc" << std::endl;
        exit(-1);
      }

      srslte_ue_sync_set_N_id_2(&cs.ue_sync, d_N_id_2); // TODO: consider if correct
      srslte_ue_sync_reset(&cs.ue_sync);

      ue_sync = &cs.ue_sync; // set initial ue_sync as cellsearch.ue_sync

      if (config.max_frames_pss && config.max_frames_pss <= cs.max_frames) {
        cs.nof_frames_to_scan = config.max_frames_pss;
      }
      if (config.threshold) {
        cs.detect_threshold = config.threshold;
      }

      cs.nof_frames_to_scan = SRSLTE_CS_DEFAULT_NOFFRAMES_TOTAL;

      // Block-specific init

      set_output_multiple(2*cs.ue_sync.frame_len);

      message_port_register_out(port_id);

      // FIXME: for now, just hard code to match unit test. Eventually this
      //        will need to be set via MSOD
      int band = 10;
      int min_earfcn = -1;
      int max_earfcn = -1;
      int nof_freqs = srslte_band_get_fd_band(band,
                                              channels,
                                              min_earfcn,
                                              max_earfcn,
                                              MAX_EARFCN);

      assert(nof_freqs > 0);
      for (int i=0; i<nof_freqs; i++)
        if ((float)channels[i].fd == (float)freq) {
          freq = i;
          break;
        }
    }

    /*
     * Our virtual destructor.
     */
    ltetrigger_c_impl::~ltetrigger_c_impl()
    {
      // FIXME: raw_pointers -> smart pointers
      srslte_ue_cellsearch_free(&cs);
    }

    int
    ltetrigger_c_impl::work(int noutput_items,
                          gr_vector_const_void_star &input_items,
                          gr_vector_void_star &output_items)
    {
      const cf_t *in = static_cast<const cf_t *>(input_items[0]);
      uint32_t nconsumed = 0;

      switch (this->state) {
      case ST_CELL_SEARCH_AND_SYNC:
      {
        ue_sync = &cs.ue_sync;

        int offset = ue_sync->time_offset;
        if (offset < 0)
          offset = -offset;

        // copy the desired number of frames to a buffer srsLTE can work with
        memcpy(input_buffer,
               &in[offset],
               sizeof(cf_t) * (ue_sync->frame_len - offset));

        uint32_t *max_N_id_2 = NULL;
        this->ue_cellsearch_scan(&cs, found_cells, max_N_id_2, input_buffer);

        if (d_nof_scanned_frames >= cs.nof_frames_to_scan) {
          if (d_nof_detected_frames) {
            get_cell(&cs, d_nof_detected_frames, &found_cells[d_N_id_2]);
            if (found_cells[d_N_id_2].psr > config.threshold) {
              // std::cout << "Entering ST_MIB_DECODE: cell_id = "
              //           << found_cells[d_N_id_2].cell_id
              //           << ", psr = " << found_cells[d_N_id_2].psr
              //           << ", threshold = " << config.threshold << std::endl;

              this->state = ST_MIB_DECODE;

              srslte_cell_t cell;
              cell.id = found_cells[d_N_id_2].cell_id;
              cell.cp = found_cells[d_N_id_2].cp;

              if (this->ue_mib_sync_init(&ue_mib, cell.id, cell.cp)) {
                // TODO: logging, exception
                std::cerr << "Error initializing ue_mib_sync" << std::endl;
                exit(-1);
              }
              ue_sync = &ue_mib.ue_sync;

            }
            d_nof_detected_frames = 0;
          }
          d_nof_scanned_frames = 0;

          if (this->state != ST_MIB_DECODE) {
            // Only increment N_id_2 if no cells were found
            d_N_id_2 = ++d_N_id_2 % 3;
            srslte_ue_sync_set_N_id_2(&cs.ue_sync, d_N_id_2);
            srslte_ue_sync_reset(&cs.ue_sync);
          }
        }

        break;
      } // case ST_CELL_SEARCH_AND_SYNC
      case ST_MIB_DECODE:
      {
        srslte_ue_cellsearch_result_t found_cell = found_cells[d_N_id_2];

        int offset = ue_sync->time_offset;
        if (offset < 0)
          offset = -offset;

        // copy the desired number of frames to a buffer srsLTE can work with
        memcpy(input_buffer,
               &in[offset],
               sizeof(cf_t) * (ue_sync->frame_len - offset));

        // std::cout << "d_nof_scanned_frames: " << d_nof_scanned_frames << std::endl;
        // std::cout << "d_nof_detected_frames: " << d_nof_detected_frames << std::endl;
        // std::cout << "d_N_id_2: " << d_N_id_2 << std::endl;

        srslte_cell_t cell = ue_sync->cell;
        uint32_t *sfn_offset = NULL;
        int mib_ret = this->ue_mib_sync_decode(&ue_mib,
                                               bch_payload,
                                               &cell.nof_ports,
                                               sfn_offset,
                                               input_buffer);

        if (mib_ret < 0) {
          // TODO: logging, exception
          std::cerr << "Error decoding MIB" << std::endl;
          exit(-1);
        }
        if (mib_ret == SRSLTE_UE_MIB_FOUND ||
            d_nof_scanned_frames >= config.max_frames_pbch) {
          this->state = ST_CELL_SEARCH_AND_SYNC;
          d_N_id_2 = ++d_N_id_2 % 3;
          srslte_ue_sync_set_N_id_2(&cs.ue_sync, d_N_id_2);
          srslte_ue_sync_reset(&cs.ue_sync);

          if (mib_ret == SRSLTE_UE_MIB_FOUND) {
            printf("Found CELL ID %d at %.1f MHz, EARFCN=%d, %d PRB, %d ports, PSS power=%.1f dBm\n",
                   cell.id,
                   channels[freq].fd,
                   channels[freq].id,
                   cell.nof_prb,
                   cell.nof_ports,
                   found_cell.peak);
            if (cell.nof_ports > 0) {
              std::memcpy(&d_results[d_nof_cells_found].cell,
                          &cell,
                          sizeof(srslte_cell_t));
              //d_results[d_nof_cells_found].freq = channels[freq].fd;
              //d_results[d_nof_cells_found].dl_earfcn = channels[freq].id;
              d_results[d_nof_cells_found].power = found_cell.peak;
              d_nof_cells_found++;

              // Report detected cell
              // TODO: use more srslte facilities to extract this information
              pmt::pmt_t msg = pmt::make_dict();
              msg = pmt::dict_add(msg, pmt::mp("link_type"), pmt::mp("downlink"));
              msg = pmt::dict_add(msg, pmt::mp("cell_id"), pmt::mp((long unsigned)cell.id));

              message_port_pub(port_id, msg);
            }
          }

          d_nof_scanned_frames = 0;
          d_nof_detected_frames = 0;
        }
        break;
      } // case ST_MIB_DECODE
      } // switch

      // A negative time offset means there are samples in buffer for the next subframe
      if (ue_sync->time_offset < 0) {
        ue_sync->time_offset = -ue_sync->time_offset;
      }

      // drop scanned samples off input buffer
      // TODO: handle case of positive offset
      int adjustment = 0;
      if (ue_sync->state == SF_TRACK) {
        if (d_make_tracking_adjustment) {
          adjustment = (ue_sync->peak_idx + ue_sync->sf_len / 2);
          d_make_tracking_adjustment = false;
        }
      }
      nconsumed = ue_sync->frame_len + adjustment - ue_sync->time_offset;

      if (nconsumed > noutput_items)
        nconsumed -= ue_sync->frame_len;

      assert(nconsumed <= noutput_items);

      // Tell runtime system how many output items we produced.
      return nconsumed;
    }

    int
    ltetrigger_c_impl::ue_mib_sync_decode(srslte_ue_mib_sync_t *q,
                                        uint8_t bch_payload[SRSLTE_BCH_PAYLOAD_LEN],
                                        uint32_t *nof_tx_ports,
                                        uint32_t *sfn_offset,
                                        cf_t *input_buffer)
    {
      int mib_ret = SRSLTE_UE_MIB_NOTFOUND;

      if (q != NULL)
      {
        mib_ret = SRSLTE_UE_MIB_NOTFOUND;
        // FIXME: should this be cs.ue_sync or q->ue_sync? Does it matter?
        int ret = this->ue_sync_buffer(&q->ue_sync, input_buffer);
        if (ret < 0) {
          fprintf(stderr, "Error calling srslte_ue_sync_work()\n");
          return -1;
        }

        if (srslte_ue_sync_get_sfidx(&q->ue_sync) == 0) {
          if (ret == 1) {
            mib_ret = srslte_ue_mib_decode(&q->ue_mib,
                                           input_buffer,
                                           bch_payload,
                                           nof_tx_ports,
                                           sfn_offset);
          } else {
            SRSLTE_DEBUG("Resetting PBCH decoder after %d frames\n",
                         q->ue_mib.frame_cnt);
            srslte_ue_mib_reset(&q->ue_mib);
          }
          d_nof_scanned_frames++;
        }
      }
      return mib_ret;
    }

    int
    ltetrigger_c_impl::ue_mib_sync_init(srslte_ue_mib_sync_t *q,
                                      uint32_t cell_id,
                                      srslte_cp_t cp)
    {
      srslte_cell_t cell;
      // If the ports are set to 0, ue_mib goes through 1, 2 and 4 ports to blindly detect nof_ports
      cell.nof_ports = 0;
      cell.id = cell_id;
      cell.cp = cp;
      cell.nof_prb = SRSLTE_UE_MIB_NOF_PRB;

      if (srslte_ue_mib_init(&q->ue_mib, cell)) {
        fprintf(stderr, "Error initiating ue_mib\n");
        return SRSLTE_ERROR;
      }
      if (this->ue_sync_init(&q->ue_sync, cell)) {
        fprintf(stderr, "Error initiating ue_sync\n");
        srslte_ue_mib_free(&q->ue_mib);
        return SRSLTE_ERROR;
      }
      srslte_ue_sync_decode_sss_on_track(&q->ue_sync, true);
      return SRSLTE_SUCCESS;
    }


    int
    ltetrigger_c_impl::ue_cellsearch_scan(srslte_ue_cellsearch_t *q,
                                        srslte_ue_cellsearch_result_t found_cells[3],
                                        uint32_t *max_N_id_2,
                                        cf_t *input_buffer)
    {
      int ret = this->ue_sync_buffer(&q->ue_sync, input_buffer);

      if (ret < 0) {
        // TODO: logging, exception
        fprintf(stderr, "Error calling srslte_ue_sync_work()\n");
        return 0;
      } else if (ret == 1) {
        /* This means a peak was found and ue_sync is now in tracking state */
        ret = srslte_sync_get_cell_id(&q->ue_sync.strack);
        if (ret >= 0) {
          if (srslte_sync_get_peak_value(&q->ue_sync.strack) > q->detect_threshold) {
            /* Save cell id, cp and peak */
            srslte_ue_cellsearch_result_t *candidate = &q->candidates[d_nof_detected_frames];
            candidate->cell_id = (uint32_t)ret;
            candidate->cp      = srslte_sync_get_cp(&q->ue_sync.strack);
            candidate->peak    = q->ue_sync.strack.pss.peak_value;
            candidate->psr     = srslte_sync_get_peak_value(&q->ue_sync.strack);
            candidate->cfo     = srslte_ue_sync_get_cfo(&q->ue_sync);

            SRSLTE_DEBUG("CELL SEARCH: [%3d/%3d/%d]: Found peak PSR=%.3f, Cell_id: %d CP: %s\n",
                         d_nof_detected_frames,
                         d_nof_scanned_frames,
                         q->nof_frames_to_scan,
                         candidate->psr,
                         candidate->cell_id,
                         srslte_cp_string(candidate->cp));

            d_nof_detected_frames++;
          }
        }
      }
      d_nof_scanned_frames++;
    }

    int
    ltetrigger_c_impl::ue_sync_buffer(srslte_ue_sync_t *q, cf_t *input_buffer) {
      int ret = SRSLTE_ERROR_INVALID_INPUTS;
      uint32_t track_idx;

      if (q               != NULL   &&
          input_buffer    != NULL)
      {
        switch (q->state) {
        case SF_FIND:
          ret = srslte_sync_find(&q->sfind, input_buffer, 0, &q->peak_idx);
          if (ret < 0) {
            fprintf(stderr, "Error finding correlation peak (%d)\n", ret);
            return SRSLTE_ERROR;
          }
          if (q->do_agc) {
            srslte_agc_process(&q->agc, input_buffer, q->sf_len);
          }

          if (ret == 1) {
            ret = find_peak_ok(q, input_buffer);
          }
          break;
        case SF_TRACK:
          ret = 1;

          srslte_sync_sss_en(&q->strack, q->decode_sss_on_track);

          q->sf_idx = (q->sf_idx + q->nof_recv_sf) % 10;

          /* Every SF idx 0 and 5, find peak around known position q->peak_idx */
          if (q->sf_idx == 0 || q->sf_idx == 5) {

            if (q->do_agc && (q->agc_period == 0 ||
                              (q->agc_period &&
                               (q->frame_total_cnt%q->agc_period) == 0)))
            {
              srslte_agc_process(&q->agc, input_buffer, q->sf_len);
            }
            track_idx = 0;

            /* track PSS/SSS around the expected PSS position */
            ret = srslte_sync_find(&q->strack, input_buffer,
                                   q->frame_len - q->sf_len/2 - q->fft_size - q->strack.max_offset/2,
                                   &track_idx);
            if (ret < 0) {
              fprintf(stderr, "Error tracking correlation peak\n");
              return SRSLTE_ERROR;
            }

            if (ret == 1) {
              ret = track_peak_ok(q, track_idx);
            } else {
              ret = track_peak_no(q);
            }
            if (ret == SRSLTE_ERROR) {
              fprintf(stderr, "Error processing tracking peak\n");
              q->state = SF_FIND;
              return SRSLTE_SUCCESS;
            }

            q->frame_total_cnt++;
          } else {
            /* Do CFO Correction if not in 0 or 5 subframes */
            if (q->correct_cfo) {
              srslte_cfo_correct(&q->sfind.cfocorr,
                                 input_buffer,
                                 input_buffer,
                                 -srslte_sync_get_cfo(&q->strack) / q->fft_size);

            }
          }

          break;
        }

      }
      return ret;
    }


    int
    ltetrigger_c_impl::find_peak_ok(srslte_ue_sync_t *q, cf_t *input_buffer) {
      if (srslte_sync_sss_detected(&q->sfind)) {
        /* Get the subframe index (0 or 5) */
        q->sf_idx = srslte_sync_get_sf_idx(&q->sfind) + q->nof_recv_sf;
      } else {
        SRSLTE_DEBUG("Found peak at %d, SSS not detected\n", q->peak_idx);
      }

      q->frame_find_cnt++;
      SRSLTE_DEBUG("Found peak %d at %d, value %.3f, Cell_id: %d CP: %s\n",
                   q->frame_find_cnt,
                   q->peak_idx,
                   srslte_sync_get_last_peak_value(&q->sfind),
                   q->cell.id,
                   srslte_cp_string(q->cell.cp));

      if (q->frame_find_cnt >= q->nof_avg_find_frames || q->peak_idx < 2*q->fft_size) {
        // SRSLTE_DEBUG("Realigning frame, reading %d samples\n", q->peak_idx+q->sf_len/2);
        // /* Receive the rest of the subframe so that we are subframe aligned*/
        // if (q->recv_callback(q->stream, input_buffer, q->peak_idx+q->sf_len/2, &q->last_timestamp) < 0) {
        //   return SRSLTE_ERROR;
        // }

        /* Reset variables */
        q->frame_ok_cnt = 0;
        q->frame_no_cnt = 0;
        q->frame_total_cnt = 0;
        q->frame_find_cnt = 0;
        q->mean_time_offset = 0;

        /* Goto Tracking state */
        SRSLTE_DEBUG("Entering tracking state, reading next %d samples\n",
                     q->peak_idx+q->sf_len/2);
        q->state = SF_TRACK;
        d_make_tracking_adjustment = true;

        /* Initialize track state CFO */
        q->strack.mean_cfo = q->sfind.mean_cfo;
        q->strack.cfo_i    = q->sfind.cfo_i;
      }

      return 0;
    }


    int
    ltetrigger_c_impl::track_peak_ok(srslte_ue_sync_t *q, uint32_t track_idx) {

      /* Make sure subframe idx is what we expect */
      if ((q->sf_idx != srslte_sync_get_sf_idx(&q->strack)) && q->decode_sss_on_track) {

        SRSLTE_DEBUG("Warning: Expected SF idx %d but got %d (%d,%g - %d,%g)!\n",
                     q->sf_idx,
                     srslte_sync_get_sf_idx(&q->strack),
                     q->strack.m0,
                     q->strack.m0_value,
                     q->strack.m1,
                     q->strack.m1_value);

        q->sf_idx = srslte_sync_get_sf_idx(&q->strack);
      }

      // Adjust time offset
      q->time_offset = ((int)track_idx - (int)q->strack.max_offset/2 - (int)q->strack.fft_size);

      if (q->time_offset) {
        SRSLTE_DEBUG("Time offset adjustment: %d samples\n", q->time_offset);
      }

      /* compute cumulative moving average time offset */
      q->mean_time_offset = (float)SRSLTE_VEC_CMA((float)q->time_offset,
                                                  q->mean_time_offset,
                                                  q->frame_total_cnt);

      /* If the PSS peak is beyond the frame (we sample too slowly),
         discard the offseted samples to align next frame */
      if (q->time_offset > 0 && q->time_offset < MAX_TIME_OFFSET) {

        SRSLTE_DEBUG("FIXME!! Positive time offset %d samples. Mean time offset %f.\n", q->time_offset, q->mean_time_offset);

        // FIXME!!
        // if (q->recv_callback(q->stream, dummy, (uint32_t) q->time_offset, &q->last_timestamp) < 0) {
        //   fprintf(stderr, "Error receiving from USRP\n");
        //   return SRSLTE_ERROR;
        // }
        q->time_offset = 0;
      }

      q->peak_idx = q->sf_len/2 + q->time_offset;
      q->frame_ok_cnt++;
      q->frame_no_cnt = 0;

      return 1;
    }


    int
    ltetrigger_c_impl::track_peak_no(srslte_ue_sync_t *q) {

      /* if we missed too many PSS go back to FIND */
      q->frame_no_cnt++;
      if (q->frame_no_cnt >= TRACK_MAX_LOST) {

        SRSLTE_DEBUG("\n%d frames lost. Going back to FIND\n",
                     (int)q->frame_no_cnt);

        q->state = SF_FIND;
      } else {
        SRSLTE_DEBUG("Tracking peak not found. Peak %.3f, %d lost\n",
                     srslte_sync_get_last_peak_value(&q->strack),
                     (int)q->frame_no_cnt);
      }

      return 1;
    }


    /* Decide the most likely cell based on the mode */
    void
    ltetrigger_c_impl::get_cell(srslte_ue_cellsearch_t *q,
                         uint32_t nof_detected_frames,
                         srslte_ue_cellsearch_result_t *found_cell)
    {
      uint32_t i, j;

      bzero(q->mode_counted, nof_detected_frames);
      bzero(q->mode_ntimes, sizeof(uint32_t) * nof_detected_frames);

      /* First find mode of CELL IDs */
      for (i = 0; i < nof_detected_frames; i++) {
        uint32_t cnt = 1;
        for (j=i+1;j<nof_detected_frames;j++) {
          if (q->candidates[j].cell_id == q->candidates[i].cell_id && !q->mode_counted[j]) {
            q->mode_counted[j]=1;
            cnt++;
          }
        }
        q->mode_ntimes[i] = cnt;
      }
      uint32_t max_times=0, mode_pos=0;
      for (i=0;i<nof_detected_frames;i++) {
        if (q->mode_ntimes[i] > max_times) {
          max_times = q->mode_ntimes[i];
          mode_pos = i;
        }
      }
      found_cell->cell_id = q->candidates[mode_pos].cell_id;
      /* Now in all these cell IDs, find most frequent CP */
      uint32_t nof_normal = 0;
      found_cell->peak = 0;
      for (i=0;i<nof_detected_frames;i++) {
        if (q->candidates[i].cell_id == found_cell->cell_id) {
          if (SRSLTE_CP_ISNORM(q->candidates[i].cp)) {
            nof_normal++;
          }
        }
        // average absolute peak value
        found_cell->peak += q->candidates[i].peak;
      }
      found_cell->peak /= nof_detected_frames;

      if (nof_normal > q->mode_ntimes[mode_pos]/2) {
        found_cell->cp = SRSLTE_CP_NORM;
      } else {
        found_cell->cp = SRSLTE_CP_EXT;
      }
      found_cell->mode = (float) q->mode_ntimes[mode_pos]/nof_detected_frames;

      // PSR is already averaged so take the last value
      found_cell->psr = q->candidates[nof_detected_frames-1].psr;

      // CFO is also already averaged
      found_cell->cfo = q->candidates[nof_detected_frames-1].cfo;
    }


    int
    ltetrigger_c_impl::ue_sync_init(srslte_ue_sync_t *q,
                                  srslte_cell_t cell)
    {
      int ret = SRSLTE_ERROR_INVALID_INPUTS;

      if (q != NULL && srslte_nofprb_isvalid(cell.nof_prb)) {
        // Init ue_sync
        std::memset(q, 0, sizeof(srslte_ue_sync_t));

        q->cell = cell;
        q->fft_size = srslte_symbol_sz(q->cell.nof_prb);
        q->sf_len = SRSLTE_SF_LEN(q->fft_size);
        q->file_mode = false;
        q->correct_cfo = true;
        q->agc_period = 0;

        if (cell.id == 1000) {
          /* If the cell is unkown, we search PSS/SSS in 5 ms */
          q->nof_recv_sf = 5;
          q->decode_sss_on_track = true;
        } else {
          /* If the cell is known, we work on a 1ms basis */
          q->nof_recv_sf = 1;
          q->decode_sss_on_track = true;
        }

        q->frame_len = q->nof_recv_sf*q->sf_len;

        if (srslte_sync_init(&q->sfind, q->frame_len, q->frame_len, q->fft_size)) {
          // TODO: throw expection, logging
          std::cerr << "Error initiating sync find" << std::endl;
          exit(-1);
        }
        if (srslte_sync_init(&q->strack, q->frame_len, TRACK_FRAME_SIZE, q->fft_size)) {
          // TODO: throw expection, logging
          std::cerr << "Error initiating sync track" << std::endl;
          exit(-1);
        }

        if (cell.id == 1000) {
          /* If the cell id is unknown, enable CP detection on find */
          srslte_sync_cp_en(&q->sfind, true);
          srslte_sync_cp_en(&q->strack, true);

          srslte_sync_set_cfo_ema_alpha(&q->sfind, 0.9);
          srslte_sync_set_cfo_ema_alpha(&q->strack, 0.4);

          srslte_sync_cfo_i_detec_en(&q->sfind, true);
          srslte_sync_cfo_i_detec_en(&q->strack, true);

          srslte_sync_set_threshold(&q->sfind, 1.5);
          q->nof_avg_find_frames = FIND_NOF_AVG_FRAMES;
          srslte_sync_set_threshold(&q->strack, 1.0);

        } else {
          srslte_sync_set_N_id_2(&q->sfind, cell.id%3);
          srslte_sync_set_N_id_2(&q->strack, cell.id%3);
          q->sfind.cp = cell.cp;
          q->strack.cp = cell.cp;
          srslte_sync_cp_en(&q->sfind, false);
          srslte_sync_cp_en(&q->strack, false);

          srslte_sync_cfo_i_detec_en(&q->sfind, true);
          srslte_sync_cfo_i_detec_en(&q->strack, true);

          srslte_sync_set_cfo_ema_alpha(&q->sfind, 0.9);
          srslte_sync_set_cfo_ema_alpha(&q->strack, 0.1);

          /* In find phase and if the cell is known, do not average pss correlation
           * because we only capture 1 subframe and do not know where the peak is.
           */
          q->nof_avg_find_frames = 1;
          srslte_sync_set_em_alpha(&q->sfind, 1);
          srslte_sync_set_threshold(&q->sfind, 4.0);

          srslte_sync_set_em_alpha(&q->strack, 0.1);
          srslte_sync_set_threshold(&q->strack, 1.3);
        }

        srslte_ue_sync_reset(q);

        ret = SRSLTE_SUCCESS;
      }
      return ret;
    }

  } /* namespace ltetrigger */
} /* namespace gr */
