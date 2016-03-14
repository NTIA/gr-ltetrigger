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

namespace gr {
  namespace ltetrigger {

    class ltetrigger_impl : public ltetrigger
    {
    private:
      int ue_sync_buffer(srslte_ue_sync_t *q);
      void get_cell(srslte_ue_cellsearch_t *q,
                    uint32_t nof_detected_frames,
                    srslte_ue_cellsearch_result_t *found_cell);
      int track_peak_no(srslte_ue_sync_t *q);
      int track_peak_ok(srslte_ue_sync_t *q, uint32_t track_idx);
      int find_peak_ok(srslte_ue_sync_t *q, cf_t *input_buffer);
      int ue_sync_init(srslte_ue_sync_t *q,
                       srslte_cell_t cell);

      const pmt::pmt_t port_id = pmt::mp("trigger");

      cell_search_cfg_t config = {
        50,   // maximum number of 5ms frames to capture for MIB decoding
        50,   // maximum number of 5ms frames to capture for PSS correlation
        4.0,  // early-stops cell detection if mean PSR is above this value
        0     // 0 or negative to disable AGC
      };

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

      srslte_ue_mib_t mib;

      uint32_t d_N_id_2 = 0; // 0, 1, or 2
      uint32_t d_nof_detected_frames = 0;
      uint32_t d_nof_scanned_frames = 0;
      uint32_t d_nconsumed = 0;

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
