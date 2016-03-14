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

#include "ltetrigger_impl.h"


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


    ltetrigger::sptr
    ltetrigger::make()
    {
      return gnuradio::get_initial_sptr(new ltetrigger_impl());
    }

    /*
     * The private constructor
     */
    ltetrigger_impl::ltetrigger_impl()
      : gr::sync_block("ltetrigger",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(0, 0, 0))
    {
      // Set debug
      srslte_verbose += 2;
      assert(SRSLTE_VERBOSE_ISDEBUG());

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
      cs.ue_sync.input_buffer = static_cast<cf_t *>(
        srslte_vec_malloc(2*cs.ue_sync.frame_len * sizeof(cf_t))
        );
      if (!cs.ue_sync.input_buffer) {
        std::cerr << "malloc" << std::endl;
        exit(-1);
      }

      srslte_ue_sync_set_N_id_2(&cs.ue_sync, d_N_id_2); // TODO: consider if correct
      srslte_ue_sync_reset(&cs.ue_sync);

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

    }

    /*
     * Our virtual destructor.
     */
    ltetrigger_impl::~ltetrigger_impl()
    {
      srslte_ue_cellsearch_free(&cs);
    }

    int
    ltetrigger_impl::work(int noutput_items,
                          gr_vector_const_void_star &input_items,
                          gr_vector_void_star &output_items)
    {
      const cf_t *in = static_cast<const cf_t *>(input_items[0]);
      uint32_t n_found_cells = 0;

      // We might get more than 1 frame (half-frame) passed in, but srsLTE
      // expects exactly one, so this is just for information... we should only
      // "consume" cs.ue_sync.frame_len samples each call to work
      uint32_t nof_frames_available = noutput_items / cs.ue_sync.frame_len;

      int offset = cs.ue_sync.time_offset;
      if (offset < 0)
        offset = -offset;

      // copy the desired number of frames to a buffer srsLTE can work with
      memcpy(cs.ue_sync.input_buffer,
             &in[offset],
             sizeof(cf_t) * (cs.ue_sync.frame_len - offset));

      float max_peak_value = -1.0;

      int ret = ue_sync_buffer(&cs.ue_sync);
      if (ret < 0) {
        fprintf(stderr, "Error calling srslte_ue_sync_work()\n");
        return 0;
      } else if (ret == 1) {
        /* This means a peak was found and ue_sync is now in tracking state */
        ret = srslte_sync_get_cell_id(&cs.ue_sync.strack);
        if (ret >= 0) {
          if (srslte_sync_get_peak_value(&cs.ue_sync.strack) > cs.detect_threshold) {
            /* Save cell id, cp and peak */
            srslte_ue_cellsearch_result_t *candidate = &cs.candidates[d_nof_detected_frames];
            candidate->cell_id = (uint32_t)ret;
            candidate->cp      = srslte_sync_get_cp(&cs.ue_sync.strack);
            candidate->peak    = cs.ue_sync.strack.pss.peak_value;
            candidate->psr     = srslte_sync_get_peak_value(&cs.ue_sync.strack);
            candidate->cfo     = srslte_ue_sync_get_cfo(&cs.ue_sync);

            SRSLTE_DEBUG("CELL SEARCH: [%3d/%3d/%d]: Found peak PSR=%.3f, Cell_id: %d CP: %s\n",
                         d_nof_detected_frames,
                         d_nof_scanned_frames,
                         cs.nof_frames_to_scan,
                         candidate->psr,
                         candidate->cell_id,
                         srslte_cp_string(candidate->cp));

            d_nof_detected_frames++;
          }
        }
      }
      d_nof_scanned_frames++;

      if (d_nof_detected_frames) {
        get_cell(&cs, d_nof_detected_frames, &found_cells[d_N_id_2]);

        // Report detected cell
        // TODO: use more srslte facilities to extract this information
        pmt::pmt_t msg = pmt::make_dict();
        msg = pmt::dict_add(msg, pmt::mp("link_type"), pmt::mp("downlink"));
        msg = pmt::dict_add(msg, pmt::mp("cell_id"), pmt::mp(369));

        message_port_pub(port_id, msg);

        n_found_cells = 0;
      }

      if (d_nof_scanned_frames >= cs.nof_frames_to_scan) {
        // FIXME: Setting a breakpoint here showed this branch is not exercised
        //        by the current unit test. It is important.
        d_nof_scanned_frames = 0;
        d_N_id_2 = ++d_N_id_2 % 3;
        srslte_ue_sync_set_N_id_2(&cs.ue_sync, d_N_id_2);
        srslte_ue_sync_reset(&cs.ue_sync);
      }

      // A negative time offset means there are samples in buffer for the next subframe
      if (cs.ue_sync.time_offset < 0) {
        cs.ue_sync.time_offset = -cs.ue_sync.time_offset;
      }

      // drop scanned samples off input buffer
      // TODO: handle case of positive offset
      int adjustment = 0;
      if (cs.ue_sync.state == SF_TRACK) {
        if (d_make_tracking_adjustment) {
          adjustment = (cs.ue_sync.peak_idx + cs.ue_sync.sf_len / 2);
          d_make_tracking_adjustment = false;
        }
      }
      d_nconsumed = cs.ue_sync.frame_len + adjustment - cs.ue_sync.time_offset;

      assert(d_nconsumed <= noutput_items);

      // Tell runtime system how many output items we produced.
      return d_nconsumed;
    }

    int
    ltetrigger_impl::ue_sync_buffer(srslte_ue_sync_t *q) {
      int ret = SRSLTE_ERROR_INVALID_INPUTS;
      uint32_t track_idx;

      if (q               != NULL   &&
          q->input_buffer != NULL)
      {
        cf_t *input_buffer = q->input_buffer;

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
    ltetrigger_impl::find_peak_ok(srslte_ue_sync_t *q, cf_t *input_buffer) {
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
    ltetrigger_impl::track_peak_ok(srslte_ue_sync_t *q, uint32_t track_idx) {

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
    ltetrigger_impl::track_peak_no(srslte_ue_sync_t *q) {

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
    ltetrigger_impl::get_cell(srslte_ue_cellsearch_t *q,
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
    ltetrigger_impl::ue_sync_init(srslte_ue_sync_t *q,
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
