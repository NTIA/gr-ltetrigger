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


#ifndef INCLUDED_LTETRIGGER_LTETRIGGER_EVENTS_H
#define INCLUDED_LTETRIGGER_LTETRIGGER_EVENTS_H

#include <string>

#include <gnuradio/block.h>
#include <pmt/pmt.h>

#include <ltetrigger/api.h>

namespace gr {
  namespace ltetrigger {

    /*!
     * \brief <+description of block+>
     * \ingroup ltetrigger
     *
     */
    class LTETRIGGER_API ltetrigger_events : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<ltetrigger_events> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of ltetrigger::ltetrigger_events.
       *
       * To avoid accidental use of raw pointers, ltetrigger::ltetrigger_events's
       * constructor is in a private implementation
       * class. ltetrigger::ltetrigger_events::make is the public interface for
       * creating new instances.
       */
      static sptr make();

      /* Return a current center frequency */
      virtual size_t fc() = 0;

      /* Manually set current center frequency. Can also use msg port 'fc'. */
      virtual void set_fc(const pmt::pmt_t &f) = 0;

      /* Return either 'uplink' or 'downlink' */
      virtual std::string linktype() = 0;

      /* Set linktype to 'downlink' */
      virtual void set_linktype_downlink() = 0;

      /* Set linktype to 'uplink' */
      virtual void set_linktype_uplink() = 0;

    };

  } // namespace ltetrigger
} // namespace gr

#endif /* INCLUDED_LTETRIGGER_LTETRIGGER_EVENTS_H */
