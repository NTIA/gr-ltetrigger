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

#ifndef INCLUDED_LTETRIGGER_LTETRIGGER_IMPL_H
#define INCLUDED_LTETRIGGER_LTETRIGGER_IMPL_H

#include <pmt/pmt.h>

#include <srslte/srslte.h>
#include <srslte/rf/rf_utils.h>

#include <ltetrigger/ltetrigger.h>


#define MAX_EARFCN 1000


namespace gr {
  namespace ltetrigger {

    class ltetrigger_impl : public ltetrigger
    {
    private:
      int ue_sync_buffer(srslte_ue_sync_t *q, cf_t *input_buffer);
      void get_cell(srslte_ue_cellsearch_t *q,
                    uint32_t nof_detected_frames,
                    srslte_ue_cellsearch_result_t *found_cell);
      int track_peak_no(srslte_ue_sync_t *q);
      int track_peak_ok(srslte_ue_sync_t *q, uint32_t track_idx);
      int find_peak_ok(srslte_ue_sync_t *q, cf_t *input_buffer);
      int ue_sync_init(srslte_ue_sync_t *q,
                       srslte_cell_t cell);
      int ue_cellsearch_scan(srslte_ue_cellsearch_t * q,
                             srslte_ue_cellsearch_result_t found_cells[3],
                             uint32_t *max_N_id_2,
                             cf_t *input_buffer);
      int ue_mib_sync_init(srslte_ue_mib_sync_t *q,
                           uint32_t cell_id,
                           srslte_cp_t cp);
      int ue_mib_sync_decode(srslte_ue_mib_sync_t *q,
                             uint8_t bch_payload[SRSLTE_BCH_PAYLOAD_LEN],
                             uint32_t *nof_tx_ports,
                             uint32_t *sfn_offset,
                             cf_t *input_buffer);

      uint32_t freq = 2400000000; // Hz, FIXME: this must be set dynamically

      enum State
      {
        ST_CELL_SEARCH_AND_SYNC,
        ST_MIB_DECODE
      };

      static State state;

      const pmt::pmt_t port_id = pmt::mp("trigger");

      // TODO: the early-stop threshold not currently implemented
      cell_search_cfg_t config = {
        10,   // max_frames_pbch - max nof 5ms frames to capture for MIB decoding
        10,   // max_frames_pss - max nof 5ms frames to capture for PSS correlation
        5.0,  // threshold - early-stops cell detection if mean PSR is above this value
        0     // init_agc - 0 or negative to disable AGC
      };

      srslte_earfcn_t channels[MAX_EARFCN];

      cf_t *input_buffer;

      struct cells {
        srslte_cell_t cell;
        float freq;
        int dl_earfcn;
        float power;
      };

      struct cells d_results[1024];

      srslte_ue_cellsearch_t cs;

      srslte_ue_cellsearch_result_t found_cells[3];

      srslte_cell_t cell;

      srslte_ue_mib_sync_t ue_mib;
      uint8_t bch_payload[SRSLTE_BCH_PAYLOAD_LEN];

      static uint32_t d_N_id_2; // 0, 1, or 2
      static uint32_t d_nof_detected_frames;
      static uint32_t d_nof_scanned_frames;
      static uint32_t d_nof_cells_found;

      bool d_make_tracking_adjustment = true;

    public:
      ltetrigger_impl();
      ~ltetrigger_impl();

      int work(int noutput_items,
               gr_vector_const_void_star &input_items,
               gr_vector_void_star &output_items);
    };

  } // namespace ltetrigger
} // namespace gr

#endif /* INCLUDED_LTETRIGGER_LTETRIGGER_IMPL_H */
