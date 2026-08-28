/***************************************************************************
 *   Copyright (c) 2026  FreeCAD contributors                              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef SKETCHERGUI_EditSlopeDialog_H
#define SKETCHERGUI_EditSlopeDialog_H

#include <QDialog>
#include <memory>

class QCheckBox;
class QLabel;
class QLineEdit;

namespace Sketcher
{
class Constraint;
class SketchObject;
}  // namespace Sketcher

namespace SketcherGui
{
class ViewProviderSketch;

/**
 * A dedicated editor for the dimensionless Slope constraint.
 *
 * A slope is a ratio dy/dx.  Following the civil-engineering convention a
 * slope is usually written as "1:n" (e.g. slope 0.5  =>  1:2).  Internally
 * FreeCAD stores the raw dy/dx ratio, so the dialog accepts either a plain
 * number ("0.5") or the "1:n" form ("1:2") and converts it to the stored
 * value (1/n) on accept.
 */
class EditSlopeDialog: public QDialog
{
    Q_OBJECT

public:
    EditSlopeDialog(ViewProviderSketch* vp, int ConstrNbr, QWidget* parent = nullptr);
    EditSlopeDialog(Sketcher::SketchObject* pcSketch, int ConstrNbr, QWidget* parent = nullptr);
    ~EditSlopeDialog() override;

    bool isSuccess() const { return success; }

private Q_SLOTS:
    void accepted();
    void rejected();
    void drivingToggled(bool);

private:
    Sketcher::SketchObject* sketch;
    Sketcher::Constraint* Constr;
    int ConstrNbr;
    bool success;
    QLineEdit* valueEdit;
    QLineEdit* nameEdit;
    QCheckBox* cbDriving;
    QLabel* errorLabel;
};

}  // namespace SketcherGui
#endif  // SKETCHERGUI_EditSlopeDialog_H
