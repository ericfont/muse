//=============================================================================
//  Awl
//  Audio Widget Library
//  $Id:$
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//=============================================================================

#include "poslabel.h"
#include "al/pos.h"

namespace Awl {

//---------------------------------------------------------
//   PosLabel
//---------------------------------------------------------

PosLabel::PosLabel(QWidget* parent)
   : 	QLabel(parent)
      {
      _smpte = false;
      setFrameStyle(WinPanel | Sunken);
      setLineWidth(2);
      setMidLineWidth(3);
      int fw = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
      setIndent(fw);
      updateValue();
      }

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize PosLabel::sizeHint() const
      {
      QFontMetrics fm(font());
      int fw = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
      int h  = fm.height() + fw * 2;
      int w;
      if (_smpte)
            w  = 2 + fm.width('9') * 9 + fm.width(':') * 3 + fw * 4;
      else
            w  = 2 + fm.width('9') * 9 + fm.width('.') * 2 + fw * 4;
      return QSize(w, h).expandedTo(QApplication::globalStrut());
      }

//---------------------------------------------------------
//   updateValue
//---------------------------------------------------------

void PosLabel::updateValue()
      {
      QString s;
      if (_smpte) {
            int min, sec, frame, subframe;
            pos.msf(&min, &sec, &frame, &subframe);
            s.sprintf("%03d:%02d:%02d:%02d", min, sec, frame, subframe);
            }
      else {
            int measure, beat, tick;
            pos.mbt(&measure, &beat, &tick);
            s.sprintf("%04d.%02d.%03u", measure+1, beat+1, tick);
            }
      setText(s);
      }

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void PosLabel::setValue(const AL::Pos& val, bool enable)
      {
      setEnabled(enable);
      pos = val;
      updateValue();
      }

//---------------------------------------------------------
//   setSmpte
//---------------------------------------------------------

void PosLabel::setSmpte(bool val)
      {
      _smpte = val;
      updateValue();
      }
}

