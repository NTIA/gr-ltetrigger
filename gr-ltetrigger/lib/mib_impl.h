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

#ifndef INCLUDED_LTETRIGGER_MIB_IMPL_H
#define INCLUDED_LTETRIGGER_MIB_IMPL_H

#include <string>
#include <vector>

#include <srslte/srslte.h>

#include <ltetrigger/mib.h>


namespace gr {
  namespace ltetrigger {

    class mib_impl : public mib
    {
    private:
      const int slot_length = 960;
      const int half_frame_length = 10 * slot_length;
      const int full_frame_length = 2 * half_frame_length;
      const int symbol_sz = 128;

      srslte_cell_t d_cell;
      srslte_ue_mib_t d_mib;

      std::vector<tag_t> d_cell_id_tags;
      std::vector<tag_t> d_cp_type_tags;

      const pmt::pmt_t cell_id_tag_key = pmt::intern("cell_id");
      const pmt::pmt_t cp_type_tag_key = pmt::intern("cp_type");


    public:
      mib_impl();
      ~mib_impl();

      int general_work(int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items);
    };

  } // namespace ltetrigger
} // namespace gr

#endif /* INCLUDED_LTETRIGGER_MIB_IMPL_H */
