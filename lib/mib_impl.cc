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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cassert>   /* assert */
#include <cstdio>    /* printf */
#include <ctime>     /* time */

#include <gnuradio/io_signature.h>
#include "mib_impl.h"


namespace gr {
  namespace ltetrigger {

    // initialize static variables
    const pmt::pmt_t mib_impl::cell_id_tag_key = pmt::intern("cell_id");

    const pmt::pmt_t mib_impl::cp_type_tag_key = pmt::intern("cp_type");

    const pmt::pmt_t
    mib_impl::tracking_lost_tag_key = pmt::intern("tracking_lost");

    const pmt::pmt_t
    mib_impl::drop_port_id = pmt::intern("drop");

    const pmt::pmt_t
    mib_impl::track_port_id = pmt::intern("track");

    mib::sptr
    mib::make(bool exit_on_success)
    {
      return gnuradio::get_initial_sptr(new mib_impl {exit_on_success});
    }

    /*
     * The private constructor
     */
    mib_impl::mib_impl(bool exit_on_success)
      : gr::block("mib",
                  gr::io_signature::make(1, 1, sizeof(cf_t)),
                  gr::io_signature::make(1, 1, sizeof(cf_t))),
        d_cell_published {false},
        d_exit_on_success {exit_on_success}
    {
      srslte_use_standard_symbol_size(true);

      // make sure d_mib is initialized so destructor doesn't cause segfault
      d_cell.nof_ports = 0; // if set to 0, detects nof_ports
      d_cell.nof_prb = SRSLTE_UE_MIB_NOF_PRB; // 6
      d_cell.cp = SRSLTE_CP_NORM;
      d_cell.id = 0;

      int rf_nof_rx_ant = 1;    // FIXME: take as block input
      int buffer_sz = 3 * SRSLTE_SF_LEN_PRB(d_cell.nof_prb);
      d_sf_buffer.resize(rf_nof_rx_ant, std::vector<cf_t>(buffer_sz));

      if (srslte_ue_mib_init(&d_mib, (cf_t **)d_sf_buffer.data(), d_cell.nof_prb))
        throw std::runtime_error {"Error initializing MIB"};

      set_tag_propagation_policy(TPP_DONT);
      set_output_multiple(half_frame_length);

      message_port_register_out(track_port_id);
      message_port_register_out(drop_port_id);
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
      const cf_t *in {static_cast<const cf_t *>(input_items[0])};
      cf_t *out {static_cast<cf_t *>(output_items[0])};

      get_tags_in_window(d_tracking_lost_tags, 0, 0, 1, tracking_lost_tag_key);

      if (!d_tracking_lost_tags.empty()) {
        if (d_cell_published)
          message_port_pub(drop_port_id, d_current_tracking_cell);

        d_cell_published = false;
        d_current_tracking_cell = pmt::PMT_NIL;
        srslte_ue_mib_reset(&d_mib);

        consume_each(half_frame_length);
        d_tracking_lost_tags.clear();
        return 0;
      }

      if (d_cell_published) {
        consume_each(half_frame_length);
        return 0;
      }

      get_tags_in_window(d_cell_id_tags, 0, 0, 1, cell_id_tag_key);
      get_tags_in_window(d_cp_type_tags, 0, 0, 1, cp_type_tag_key);

      // sanity check - if this trips often you're setting psr_threshold
      //                too low and sending junk through SSS - use >= 1.5
      if (d_cell_id_tags.size() != 1 || d_cp_type_tags.size() != 1) {
        consume_each(half_frame_length);
        return 0;
      }

      unsigned int cell_id = pmt::to_long(d_cell_id_tags[0].value);
      srslte_cp_t cp;

      if (d_cp_type_tags[0].value == pmt::PMT_T)
        cp = SRSLTE_CP_NORM;
      else
        cp = SRSLTE_CP_EXT;

      d_cell_id_tags.clear();
      d_cp_type_tags.clear();

      if (cell_id != d_cell.id || cp != d_cell.cp) {
        // reinit MIB
        srslte_ue_mib_reset(&d_mib);
        d_cell.id = cell_id;
        d_cell.cp = cp;

        if (srslte_ue_mib_init(&d_mib, (cf_t **)d_sf_buffer.data(), d_cell.nof_prb)) {
          // SSS block passed us non-sense
          consume_each(half_frame_length);
          return 0;
        }
      }

      int ret {srslte_ue_mib_decode(&d_mib,
                                    d_bch_payload,
                                    &d_cell.nof_ports,
                                    &d_sfn_offset)};

      if (ret == SRSLTE_UE_MIB_FOUND) {
        srslte_pbch_mib_unpack(d_bch_payload,
                               &d_cell,
                               reinterpret_cast<uint32_t *>(&d_sfn_offset));
        //srslte_cell_fprint(stdout, &d_cell, sfn_offset);
        d_current_tracking_cell = pack_cell(d_cell, d_sfn_offset);
        message_port_pub(track_port_id, d_current_tracking_cell);
        d_cell_published = true;
        if (d_exit_on_success)
          return WORK_DONE;
      }

      consume_each(noutput_items);

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

    pmt::pmt_t
    mib_impl::pack_cell(const srslte_cell_t &cell, const int &sfn) const
    {
      pmt::pmt_t d {pmt::make_dict()};

      d = pmt::dict_add(d,
                        pmt::intern("cell_id"),
                        pmt::from_long(cell.id));

      d = pmt::dict_add(d,
                        pmt::intern("nof_tx_ports"),
                        pmt::from_long(cell.nof_ports));

      std::string cp_len_str;
      if (cell.cp == SRSLTE_CP_NORM)
        cp_len_str = "Normal";
      else
        cp_len_str = "Extended";

      d = pmt::dict_add(d,
                        pmt::intern("cp_len"),
                        pmt::intern(cp_len_str));

      d = pmt::dict_add(d,
                        pmt::intern("nof_prb"),
                        pmt::from_long(cell.nof_prb));

      std::string phich_len_str;
      if (cell.phich_length == SRSLTE_PHICH_NORM)
        phich_len_str = "Normal";
      else
        phich_len_str = "Extended";

      d = pmt::dict_add(d,
                        pmt::intern("phich_len"),
                        pmt::intern(phich_len_str));

      std::string nof_phich_resources_str;
      switch (cell.phich_resources) {
      case SRSLTE_PHICH_R_1_6:
        nof_phich_resources_str = "1/6";
        break;
      case SRSLTE_PHICH_R_1_2:
        nof_phich_resources_str = "1/2";
        break;
      case SRSLTE_PHICH_R_1:
        nof_phich_resources_str = "1";
        break;
      case SRSLTE_PHICH_R_2:
        nof_phich_resources_str = "2";
        break;
      }

      d = pmt::dict_add(d,
                        pmt::intern("nof_phich_resources"),
                        pmt::intern(nof_phich_resources_str));

      d = pmt::dict_add(d,
                        pmt::intern("sfn_offset"),
                        pmt::from_long(sfn));

      d = pmt::dict_add(d,
                        pmt::intern("tracking_start_time"),
                        pmt::from_long(std::time(0)));

      return d;
    }

  } /* namespace ltetrigger */
} /* namespace gr */
