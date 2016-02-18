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
      message_port_register_out(port_id);

      std::memset(&cs, 0, sizeof(srslte_ue_cellsearch_t));
      cs.max_frames = max_frames;
      cs.nof_frames_to_scan = SRSLTE_CS_DEFAULT_NOFFRAMES_TOTAL;
      cs.detect_threshold = 1.0;

      std::memset(found_cells, 0, 3*sizeof(srslte_ue_cellsearch_result_t));

      std::memset(&cell, 0, sizeof(srslte_cell_t));
      cell.id = SRSLTE_CELL_ID_UNKNOWN;
      cell.nof_prb = SRSLTE_CS_NOF_PRB;

      cs.candidates = static_cast<srslte_ue_cellsearch_result_t *>(
        calloc(max_frames, sizeof(srslte_ue_cellsearch_result_t))
        );
      cs.mode_ntimes = static_cast<uint32_t *>(
        calloc(max_frames, sizeof(uint32_t))
        );
      cs.mode_counted = static_cast<uint8_t *>(
        calloc(max_frames, sizeof(uint8_t))
        );

      // Init ue_sync
      // https://github.com/srsLTE/srsLTE/blob/master/srslte/lib/ue/src/ue_sync.c
      std::memset(&cs.ue_sync, 0, sizeof(srslte_ue_sync_t));
      cs.ue_sync.cell = cell;
      cs.ue_sync.fft_size = srslte_symbol_sz(cs.ue_sync.cell.nof_prb);
      cs.ue_sync.sf_len = SRSLTE_SF_LEN(cs.ue_sync.fft_size);
      cs.ue_sync.file_mode = false;
      cs.ue_sync.correct_cfo = true;
      cs.ue_sync.agc_period = 0;
      // If the cell is unkown, we search PSS/SSS in 5 ms
      cs.ue_sync.nof_recv_sf = 5;
      cs.ue_sync.decode_sss_on_track = true;
      cs.ue_sync.frame_len = cs.ue_sync.nof_recv_sf*cs.ue_sync.sf_len;

      // Init sync
      if (srslte_sync_init(&cs.ue_sync.sfind,
                           cs.ue_sync.frame_len,
                           cs.ue_sync.frame_len,
                           cs.ue_sync.fft_size))
      {
        std::cerr << "Error initiating sync find" << std::endl;
        exit(-1);
      }
      if (srslte_sync_init(&cs.ue_sync.strack,
                           cs.ue_sync.frame_len,
                           TRACK_FRAME_SIZE,
                           cs.ue_sync.fft_size))
      {
        std::cerr << "Error initiating sync find" << std::endl;
        exit(-1);
      }

      // If the cell id is unknown, enable CP detection on find
      srslte_sync_cp_en(&cs.ue_sync.sfind, true);
      srslte_sync_cp_en(&cs.ue_sync.strack, true);

      srslte_sync_set_cfo_ema_alpha(&cs.ue_sync.sfind, 0.9);
      srslte_sync_set_cfo_ema_alpha(&cs.ue_sync.strack, 0.4);

      srslte_sync_cfo_i_detec_en(&cs.ue_sync.sfind, true);
      srslte_sync_cfo_i_detec_en(&cs.ue_sync.strack, true);

      srslte_sync_set_threshold(&cs.ue_sync.sfind, 1.5);
      cs.ue_sync.nof_avg_find_frames = FIND_NOF_AVG_FRAMES;
      srslte_sync_set_threshold(&cs.ue_sync.strack, 1.0);

      // FIXME: getting swig undefined symbol issue with srslte_vec_malloc,
      //        so inlining. Also, consider if AVX compatibility is something
      //        we could drop for our purposes:

      /* SRSLTE Note: We align memory to 32 bytes (for AVX compatibility)
       * because in some cases volk can incorrectly detect the architecture.
       This could be inefficient for SSE or non-SIMD platforms but shouldn't
       * be a huge problem.
       */
      cs.ue_sync.input_buffer = static_cast<cf_t *>(
        srslte_vec_malloc(2*cs.ue_sync.frame_len * sizeof(cf_t))
        );
      if (!cs.ue_sync.input_buffer) {
        std::cerr << "malloc" << std::endl;
        exit(-1);
      }

      if (config.max_frames_pss && config.max_frames_pss <= cs.max_frames) {
        cs.nof_frames_to_scan = config.max_frames_pss;
      }
      if (config.threshold) {
        cs.detect_threshold = config.threshold;
      }

    }

    /*
     * Our virtual destructor.
     */
    ltetrigger_impl::~ltetrigger_impl()
    {}

    int
    ltetrigger_impl::work(int noutput_items,
                          gr_vector_const_void_star &input_items,
                          gr_vector_void_star &output_items)
    {
      const cf_t *in = static_cast<const cf_t *>(input_items[0]);

      float max_peak_value = -1.0;
      uint32_t nof_detected_cells = 0;
      for (uint32_t N_id_2=0, ret=0; N_id_2<3 && ret >= 0; N_id_2++) {
        // TODO:
        // ret = ue_cellsearch_scan_N_id_2(&cs,
        //                                 N_id_2,
        //                                 &found_cells[N_id_2]);
        if (ret < 0) {
          std::cerr << "Error searching cell" << std::endl;
          return ret;
        }
        nof_detected_cells += ret;
        // if (max_N_id_2) {
        //   if (found_cells[N_id_2].peak > max_peak_value) {
        //     max_peak_value = found_cells[N_id_2].peak;
        //     *max_N_id_2 = N_id_2;
        //   }
        // }
      }

      pmt::pmt_t msg = pmt::make_dict();
      msg = pmt::dict_add(msg, pmt::mp("link_type"), pmt::mp("downlink"));
      msg = pmt::dict_add(msg, pmt::mp("cell_id"), pmt::mp(369));

      message_port_pub(port_id, msg);

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace ltetrigger */
} /* namespace gr */
