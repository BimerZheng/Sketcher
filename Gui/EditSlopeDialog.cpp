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

#include <cmath>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStringList>
#include <QVBoxLayout>

#include <App/Document.h>
#include <Base/Console.h>
#include <Gui/CommandT.h>
#include <Gui/Notifications.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "EditDatumDialog.h"
#include "EditSlopeDialog.h"
#include "Utils.h"
#include "ViewProviderSketch.h"

using namespace SketcherGui;

/* TRANSLATOR SketcherGui::EditSlopeDialog */

EditSlopeDialog::EditSlopeDialog(ViewProviderSketch* vp, int ConstrNbr, QWidget* parent)
    : EditSlopeDialog(vp->getSketchObject(), ConstrNbr, parent)
{
}

EditSlopeDialog::EditSlopeDialog(Sketcher::SketchObject* pcSketch, int ConstrNbr, QWidget* parent)
    : QDialog(parent)
    , sketch(pcSketch)
    , ConstrNbr(ConstrNbr)
    , success(false)
{
    const std::vector<Sketcher::Constraint*>& Constraints = sketch->Constraints.getValues();
    if (ConstrNbr >= 0 && ConstrNbr < static_cast<int>(Constraints.size())) {
        Constr = Constraints[ConstrNbr];
    }
    else {
        Constr = nullptr;
    }

    setWindowModality(Qt::ApplicationModal);
    setWindowTitle(tr("Insert Slope"));

    auto* mainLayout = new QVBoxLayout(this);
    auto* grid = new QGridLayout();

    auto* valueLabel = new QLabel(tr("Slope (1:n):"), this);
    valueEdit = new QLineEdit(this);
    valueEdit->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    valueEdit->setToolTip(tr("Enter the slope as a ratio.  Use \"1:n\" form\n"
                             "(e.g. 1:2  =>  slope 0.5) or a plain number\n"
                             "(e.g. 0.5)."));
    grid->addWidget(valueLabel, 0, 0);
    grid->addWidget(valueEdit, 0, 1);

    auto* nameLabel = new QLabel(tr("Name"), this);
    nameEdit = new QLineEdit(this);
    nameEdit->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    nameEdit->setToolTip(tr("Constraint name (available for expressions)"));
    grid->addWidget(nameLabel, 1, 0);
    grid->addWidget(nameEdit, 1, 1);

    mainLayout->addLayout(grid);

    cbDriving = new QCheckBox(tr("Reference"), this);
    cbDriving->setToolTip(tr("Reference (or constraint) dimension"));
    mainLayout->addWidget(cbDriving);

    errorLabel = new QLabel(this);
    errorLabel->setWordWrap(true);
    errorLabel->setStyleSheet(QStringLiteral("color: #cc0000;"));
    errorLabel->hide();
    mainLayout->addWidget(errorLabel);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                           Qt::Horizontal,
                                           this);
    mainLayout->addWidget(buttonBox);

    // Initial values
    double datum = 0.0;
    if (Constr) {
        datum = Constr->getValue();
        nameEdit->setText(QString::fromStdString(Constr->Name));
        cbDriving->setChecked(!Constr->isDriving);
    }

    // Show current slope as "1:n" (n = 1/slope).  The sign of the slope is
    // shown before "1" (e.g. -1:1 rather than 1:-1) so it is unambiguous.
    if (std::abs(datum) > 1e-12) {
        const double n = std::abs(1.0 / datum);
        const QString sign = (datum < 0.0) ? QStringLiteral("-") : QString();
        valueEdit->setText(sign + QStringLiteral("1:%1").arg(n));
    }
    else {
        valueEdit->setText(QStringLiteral("0"));
    }

    connect(buttonBox, &QDialogButtonBox::accepted, this, &EditSlopeDialog::accepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &EditSlopeDialog::rejected);
    connect(cbDriving, &QCheckBox::toggled, this, &EditSlopeDialog::drivingToggled);

    // Give focus to the value field and select the number part.
    valueEdit->selectAll();
    valueEdit->setFocus();
}

EditSlopeDialog::~EditSlopeDialog() = default;

void EditSlopeDialog::drivingToggled(bool checked)
{
    // "Reference" is checked => the constraint is driven (not driving), so the
    // value cannot be edited.  When unchecked the constraint is driving and
    // the value field is editable.
    valueEdit->setEnabled(!checked);
    sketch->setDriving(ConstrNbr, !checked);
    if (!sketch->noRecomputes) {  // if noRecomputes, solve() is already done by setDriving()
        sketch->solve();
    }
}

void EditSlopeDialog::accepted()
{
    if (!Constr) {
        Gui::Command::abortCommand();
        QDialog::reject();
        return;
    }

    double newDatum = 0.0;
    QString text = valueEdit->text().trimmed();
    bool ok = false;

    if (text.contains(QLatin1Char(':'))) {
        // "1:n" form.  Parse the value after the colon.
        QStringList parts = text.split(QLatin1Char(':'));
        bool numOk = false;
        double n = parts.value(1).toDouble(&numOk);
        // Sanity check: numerator is conventionally 1, but tolerate a ratio a:b
        // by using value = a/b (i.e. dy/dx).  If the first part is "1" keep the
        // engineering meaning n in "1:n" (so datum = 1/n).
        if (numOk) {
            bool firstOk = false;
            double first = parts.value(0).toDouble(&firstOk);
            if (firstOk && std::abs(first) > 1e-12) {
                // datum = first / n  (works for "1:2" => 0.5, "1:0.5" => 2)
                newDatum = first / n;
            }
            else {
                newDatum = 1.0 / n;
            }
            ok = true;
        }
    }
    else {
        // Plain number => direct dy/dx value.
        newDatum = text.toDouble(&ok);
    }

    if (!ok || !std::isfinite(newDatum) || std::abs(newDatum) < 1e-12) {
        errorLabel->setText(
            tr("Invalid slope.  Use the \"1:n\" form (e.g. 1:2) or a plain number "
               "(e.g. 0.5).  The value must be finite and non-zero."));
        errorLabel->show();
        valueEdit->setFocus();
        return;
    }

    try {
        // "Reference" checked => driven (value is not editable, already handled by
        // drivingToggled).  Only when the constraint is driving do we update the datum.
        if (!cbDriving->isChecked()) {
            Gui::cmdAppObjectArgs(
                sketch,
                "setDatum(%i,App.Units.Quantity('%f'))",
                ConstrNbr,
                newDatum
            );
        }

        std::string constraintName = nameEdit->text().trimmed().toStdString();
        std::string currConstraintName = sketch->Constraints[ConstrNbr]->Name;

        if (constraintName != currConstraintName) {
            if (!SketcherGui::checkConstraintName(sketch, constraintName)) {
                constraintName = currConstraintName;
            }

            Gui::cmdAppObjectArgs(
                sketch,
                "renameConstraint(%d, u'%s')",
                ConstrNbr,
                constraintName.c_str()
            );
        }

        Gui::Command::commitCommand();

        sketch->solve();
        tryAutoRecompute(sketch);
        success = true;
        QDialog::accept();
    }
    catch (const Base::Exception& e) {
        Gui::NotifyUserError(sketch,
                             QT_TRANSLATE_NOOP("Notifications", "Value Error"),
                             e.what());
        Gui::Command::abortCommand();
        if (sketch->noRecomputes) {
            sketch->solve();
        }
        QDialog::reject();
    }
}

void EditSlopeDialog::rejected()
{
    Gui::Command::abortCommand();
    sketch->recomputeFeature();
    QDialog::reject();
}
