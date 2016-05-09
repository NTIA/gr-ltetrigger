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

#ifndef INCLUDED_LTETRIGGER_CFO_IMPL_H
#define INCLUDED_LTETRIGGER_CFO_IMPL_H

#include <ltetrigger/cfo.h>

namespace gr {
  namespace ltetrigger {

    class cfo_impl : public cfo
    {
    private:
      srslte_cfo_t cfocorr;

      const int half_frame_length = 9600; // 10 slots = 1 half frame
      const int full_frame_length = 2 * frame_length;
      const int symbol_sz = 128;

      float d_fc;
      const float nofc = -9999.0;

    public:
      cfo_impl();
      ~cfo_impl();

      int work(int noutput_items,
               gr_vector_const_void_star &input_items,
               gr_vector_void_star &output_items);

      void set_fc(float fc) { d_fc = fc; }
      float fc() { return d_fc; }
      void disable() { d_fc = nofc; }
      bool is_enabled() { return d_fc != nofc; }
    };

  } // namespace ltetrigger
} // namespace gr

#endif /* INCLUDED_LTETRIGGER_CFO_IMPL_H */
