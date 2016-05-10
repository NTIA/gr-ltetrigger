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

#ifndef INCLUDED_LTETRIGGER_PSS_IMPL_H
#define INCLUDED_LTETRIGGER_PSS_IMPL_H

#include <future> /* future */
#include <string> /* string */

#include <srslte/srslte.h>

#include <ltetrigger/pss.h>


namespace gr {
  namespace ltetrigger {

    class pss_impl : public pss
    {
    private:
      srslte_pss_synch_t d_pss[3]; // one for each N_id_2
      float d_peak_values[3];
      std::future<int> d_peak_pos[3];

      const int half_frame_length = 9600; // 10 slots = 1 half frame
      const int full_frame_length = 2 * half_frame_length;
      const int symbol_sz = 128;
      const std::string length_tag_key = "frame_length";
      const std::string N_id_2_tag_key = "N_id_2";

      static float d_max_peak_value;

      float d_peak_threshold = 5.0;

      int d_N_id_2;

    public:
      pss_impl();
      ~pss_impl();

      int general_work(int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items);
    };

  } // namespace ltetrigger
} // namespace gr

#endif /* INCLUDED_LTETRIGGER_PSS_IMPL_H */
