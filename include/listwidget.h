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

#ifndef __LISTWIDGET_H__
#define __LISTWIDGET_H__
#include <QListWidget>

// list widgets with duplicate check on drop
class ListWidget: public QListWidget {

public:
    using QListWidget::QListWidget;

    // body in source file to avoid using logging n header (multiple log macro definitions)
    void dropEvent(QDropEvent *event);
};
#endif /* __LISTWIDGET_H__ */
