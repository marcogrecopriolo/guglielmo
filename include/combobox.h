/*
 *    Copyright (C) 2023
 *    Marco Greco <marcogrecopriolo@gmail.com>
 *
 *    This file is part of the guglielmo FM DAB tuner software package.
 *
 *    guglielmo is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    guglielmo is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with guglielmo; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __COMBOBOX_H__
#define __COMBOBOX_H__
#include <QComboBox>

// combobox with centered listview
class ComboBox: public QComboBox {

public:
    using QComboBox::QComboBox;

    void showPopup() {
        QComboBox::showPopup();
	int diff = view()->width() - width();
        QPoint pos = mapToGlobal(QPoint(-diff / 2, height()));
        view()->parentWidget()->move(pos);
    }
};
#endif /* __COMBOBOX_H__ */
