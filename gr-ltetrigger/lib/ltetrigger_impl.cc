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

#include <gnuradio/io_signature.h>
#include <pmt/pmt.h>

#include "ltetrigger_impl.h"

namespace gr {
  namespace ltetrigger {

    ltetrigger::sptr
    ltetrigger::make()
    {
      return gnuradio::get_initial_sptr
        (new ltetrigger_impl());
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
      auto in = static_cast<const gr_complex *>(input_items[0]);

      pmt::pmt_t msg = pmt::make_dict();
      msg = pmt::dict_add(msg, pmt::mp("link_type"), pmt::mp("downlink"));
      msg = pmt::dict_add(msg, pmt::mp("cell_id"), pmt::mp(369));

      message_port_pub(port_id, msg);

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace ltetrigger */
} /* namespace gr */
