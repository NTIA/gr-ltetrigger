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

#ifndef INCLUDED_LTETRIGGER_SSS_IMPL_H
#define INCLUDED_LTETRIGGER_SSS_IMPL_H

#include <srslte/srslte.h>

#include <ltetrigger/sss.h>


namespace gr {
  namespace ltetrigger {

    class sss_impl : public sss
    {
    private:
      srslte_sync_t d_sync; // one for each N_id_2
      int d_N_id_2;
      int d_subframe_idx = -1;

      static const int slot_length = 960;
      static const int half_frame_length = 10 * slot_length;
      static const int full_frame_length = 2 * half_frame_length;
      static const int symbol_sz = 128;
      static const int max_offset = 100; // FIXME: set arbitrarily, does it matter?

      const pmt::pmt_t cell_id_tag_key = pmt::intern("cell_id");
      const pmt::pmt_t cp_type_tag_key = pmt::intern("cp_type");

    public:
      sss_impl(int N_id_2);
      ~sss_impl();

      int work(int noutput_items,
               gr_vector_const_void_star &input_items,
               gr_vector_void_star &output_items);
    };

  } // namespace ltetrigger
} // namespace gr

#endif /* INCLUDED_LTETRIGGER_SSS_IMPL_H */
