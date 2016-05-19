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

#include <cassert>
#include <iostream>

#include <gnuradio/io_signature.h>
#include "mib_impl.h"


namespace gr {
  namespace ltetrigger {

    mib::sptr
    mib::make()
    {
      return gnuradio::get_initial_sptr(new mib_impl());
    }

    /*
     * The private constructor
     */
    mib_impl::mib_impl()
      : gr::block("mib",
                  gr::io_signature::make(1, 1, sizeof(cf_t)),
                  gr::io_signature::make(1, 1, sizeof(cf_t)))
    {
      srslte_use_standard_symbol_size(true);

      // make sure d_mib is initialized so that destructor call doesn't segfault
      d_cell.nof_ports = 0; // if set to 0, detects nof_ports
      d_cell.nof_prb = SRSLTE_UE_MIB_NOF_PRB; // 6
      d_cell.cp = SRSLTE_CP_NORM;
      d_cell.id = 100;

      if (srslte_ue_mib_init(&d_mib, d_cell)) {
        std::cerr << "Error initializing MIB" << std::endl;
        exit(EXIT_FAILURE);
      }

      set_tag_propagation_policy(TPP_DONT);
      set_output_multiple(half_frame_length);
    }

    /*
     * Our virtual destructor.
     */
    mib_impl::~mib_impl()
    {
      srslte_ue_mib_free(&d_mib);
    }

    int
    mib_impl::general_work(int noutput_items,
                           gr_vector_int &ninput_items,
                           gr_vector_const_void_star &input_items,
                           gr_vector_void_star &output_items)
    {
      const cf_t *in = static_cast<const cf_t *>(input_items[0]);
      cf_t *out = static_cast<cf_t *>(output_items[0]);

      d_cell_id_tags.clear();
      d_cp_type_tags.clear();

      get_tags_in_window(d_cell_id_tags, 0, 0, 1, cell_id_tag_key);
      get_tags_in_window(d_cp_type_tags, 0, 0, 1, cp_type_tag_key);

      // sanity check
      assert(d_cell_id_tags.size() == 1);
      assert(d_cp_type_tags.size() == 1);

      unsigned int cell_id = pmt::to_long(d_cell_id_tags[0].value);
      srslte_cp_t cp;

      if (d_cp_type_tags[0].value == pmt::PMT_T)
        cp = SRSLTE_CP_NORM;
      else
        cp = SRSLTE_CP_EXT;

      if (cell_id != d_cell.id || cp != d_cell.cp) {
        // reinit MIB
        srslte_ue_mib_reset(&d_mib);
        d_cell.id = cell_id;
        d_cell.cp = cp;
        if (srslte_ue_mib_init(&d_mib, d_cell)) {
          std::cerr << "Error initializing MIB" << std::endl;
          exit(EXIT_FAILURE);
        }
        printf("DEBUG: Reinitializing mib with cell_id %d, cp_type: %s\n",
               d_cell.id, srslte_cp_string(d_cell.cp));
      }

      uint8_t bch_payload[SRSLTE_BCH_PAYLOAD_LEN];
      int sfn_offset;

      int ret = srslte_ue_mib_decode(&d_mib,
                                     const_cast<cf_t *>(in),
                                     bch_payload,
                                     &d_cell.nof_ports,
                                     &sfn_offset);

      if (ret == SRSLTE_UE_MIB_FOUND) {
        srslte_pbch_mib_unpack(bch_payload, &d_cell, reinterpret_cast<uint32_t *>(&sfn_offset));
        srslte_cell_fprint(stdout, &d_cell, sfn_offset);
        return -1;
      }

      consume_each(noutput_items);

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace ltetrigger */
} /* namespace gr */
