/* -*- c++ -*- */
/*
 * Copyright 2016 Institute for Telecommunication Sciences.
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

#ifndef INCLUDED_LTETRIGGER_MIB_IMPL_H
#define INCLUDED_LTETRIGGER_MIB_IMPL_H

#include <mutex>
#include <vector>

#include <pmt/pmt.h>

#include <srslte/srslte.h>

#include <ltetrigger/mib.h>


namespace gr {
  namespace ltetrigger {

    class mib_impl : public mib
    {
    private:
      pmt::pmt_t pack_cell(const srslte_cell_t &cell, const int &sfn) const;

      static const int slot_length {960};
      static const int half_frame_length {10 * slot_length};
      static const int full_frame_length {2 * half_frame_length};
      static const int symbol_sz {128};

      static const pmt::pmt_t cell_id_tag_key;
      static const pmt::pmt_t cp_type_tag_key;
      static const pmt::pmt_t tracking_lost_tag_key;

      static const pmt::pmt_t drop_port_id;
      static const pmt::pmt_t track_port_id;

      pmt::pmt_t d_current_tracking_cell {pmt::PMT_NIL};

      bool d_cell_published;

      srslte_cell_t d_cell;
      srslte_ue_mib_t d_mib;

      std::vector<tag_t> d_tracking_lost_tags;
      std::vector<tag_t> d_cell_id_tags;
      std::vector<tag_t> d_cp_type_tags;

      bool d_exit_on_success;
      int d_sfn_offset;
      uint8_t d_bch_payload[SRSLTE_BCH_PAYLOAD_LEN] {0};

    public:
      mib_impl(bool exit_on_success);
      ~mib_impl();

      int general_work(int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items);

    };

  } // namespace ltetrigger
} // namespace gr

#endif /* INCLUDED_LTETRIGGER_MIB_IMPL_H */
