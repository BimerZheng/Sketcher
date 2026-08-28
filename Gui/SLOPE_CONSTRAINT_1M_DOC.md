# 坡度（Slope）约束工程化 1:m 显示 —— 修改文档

> 本文档记录为让 **Sketch 的 Slope 约束** 遵循土木工程惯例以 `1:m` 格式显示所做的全部最终修改。
> 只记录最后成功运行的改动，含**完整新增代码**，不含中间过程。
> 每个改动均标注了**所在文件、所在函数与具体行号**。

---

## 一、背景与目标

FreeCAD 的 `Sketcher::Slope` 约束内部存储的是 **无量纲的 dy/dx 比值**（如 `0.5`）。
工程中坡度通常用 **`1:m`** 表示（如坡度 `0.5` 显示为 `1:2`）。

改造目标：
1. 约束面板列表中的坡度显示为 `(1:m)`。
2. 3D 视图中的坡度标签显示为 `1:m`。
3. 双击坡度约束时弹出 **独立的专用编辑对话框**（仿照 `HorizontalAlignment` 的方式），
   接受 `1:m` 输入（如 `1:2`）或普通数值（如 `0.5`）。
4. **负坡度** 的符号显示在 `1` 之前（如 `-1:1`，而非 `1:-1`）。

---

## 二、文件改动总览

| 操作 | 文件 |
|------|------|
| **新增** | `src/Mod/Sketcher/Gui/EditSlopeDialog.h` |
| **新增** | `src/Mod/Sketcher/Gui/EditSlopeDialog.cpp` |
| **修改** | `src/Mod/Sketcher/Gui/CMakeLists.txt` |
| **修改** | `src/Mod/Sketcher/Gui/CommandConstraints.cpp` |
| **修改** | `src/Mod/Sketcher/Gui/EditDatumDialog.cpp` |
| **修改** | `src/Mod/Sketcher/Gui/TaskSketcherConstraints.cpp` |
| **修改** | `src/Mod/Sketcher/Gui/EditModeConstraintCoinManager.cpp` |
| **修改** | `src/Mod/Sketcher/Gui/ViewProviderSketch.cpp` |

---

## 三、新增文件（完整代码）

### 1. `EditSlopeDialog.h`（新增，完整内容）

**文件**：`src/Mod/Sketcher/Gui/EditSlopeDialog.h`

```cpp
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
```

---

### 2. `EditSlopeDialog.cpp`（新增，完整内容）

**文件**：`src/Mod/Sketcher/Gui/EditSlopeDialog.cpp`

```cpp
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
```

---

## 四、修改文件（含精确位置）

### 3. `CMakeLists.txt`（修改）

**文件**：`src/Mod/Sketcher/Gui/CMakeLists.txt`
**位置**：源码列表 `EditDatumDialog.h`（第 136 行）之后，`PropertyVisualLayerList.cpp`（第 139 行）之前

```cmake
    EditDatumDialog.cpp
    EditDatumDialog.h
    EditSlopeDialog.cpp      # 新增
    EditSlopeDialog.h        # 新增
    PropertyVisualLayerList.cpp
```

---

### 4. `CommandConstraints.cpp`（修改）

**文件**：`src/Mod/Sketcher/Gui/CommandConstraints.cpp`

#### 4.1 新增 include

**位置**：第 50 行 `#include "EditDatumDialog.h"` 之后

```cpp
#include "CommandConstraints.h"
#include "DrawSketchHandler.h"
#include "EditDatumDialog.h"
#include "EditSlopeDialog.h"    // 新增
#include "Utils.h"
#include "ViewProviderSketch.h"
```

#### 4.2 `finishDatumConstraint()` —— 创建坡度约束后弹出专用对话框

**位置**：函数 `finishDatumConstraint`（第 95 行）内，"Ask for the value of the distance immediately" 处（第 156 行）

原代码只有 `EditDatumDialog`，本次改为按类型分流：

```cpp
    // Ask for the value of the distance immediately
    if (show && isDriving) {
        if (lastConstraintType == Sketcher::Slope) {
            // Slope is a dimensionless ratio; use the dedicated "1:n" editor.
            EditSlopeDialog editSlopeDialog(sketch, ConStr.size() - 1);
            editSlopeDialog.exec();
        }
        else {
            EditDatumDialog editDatumDialog(sketch, ConStr.size() - 1);
            editDatumDialog.exec();
        }
    }
    else {
        // no dialog was shown so commit the command
        cmd->commitCommand();
    }
```

#### 4.3 Slope 命令 `CmdSketcherConstrainSlope`（完整记录）

**位置**：`class CmdSketcherConstrainSlope` 定义于第 9026 行。

本次重写了其 `applyConstraint()` 方法，使其按 **`slope = dy / |dx|`**（与 GCS 求解器
`ConstraintSlope::error() = dy - s*|dx|` 一致）计算有符号坡度，并针对垂直线给出防护：

```cpp
class CmdSketcherConstrainSlope: public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainSlope();
    ~CmdSketcherConstrainSlope() override
    {}
    const char* className() const override
    {
        return "CmdSketcherConstrainSlope";
    }

protected:
    void applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex) override;
};

CmdSketcherConstrainSlope::CmdSketcherConstrainSlope()
    : CmdSketcherConstraint("Sketcher_ConstrainSlope")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Slope Constraint");
    sToolTipText = QT_TR_NOOP("Fixes the slope (dy/dx) of the selected line");
    sWhatsThis = "Sketcher_ConstrainSlope";
    sStatusTip = sToolTipText;
    sPixmap = "Constraint_Slope";
    eType = ForEdit;

    // Either a single line/edge, or two points (a construction line through the
    // two points is added and its slope is constrained).
    allowedSelSequences = {{SelEdge}, {SelVertexOrRoot, SelVertexOrRoot}};
}

void CmdSketcherConstrainSlope::applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex)
{
    Q_UNUSED(seqIndex);

    if (selSeq.size() != 1) {
        return;
    }

    SketcherGui::ViewProviderSketch* sketchgui =
        static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    if (!sketchgui) {
        return;
    }
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    const int GeoId1 = selSeq[0].GeoId;
    const Sketcher::PointPos PosId1 = selSeq[0].PosId;

    if (!isEdge(GeoId1, PosId1)) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("Select a line from the sketch."));
        return;
    }

    if (isBsplinePole(Obj, GeoId1)) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("Select an edge that is not a B-spline weight."));
        return;
    }

    const Part::Geometry* geom = Obj->getGeometry(GeoId1);
    if (!(geom && isLineSegment(*geom))) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("Slope constraint can only be applied to a line segment."));
        return;
    }

    auto lineSeg = static_cast<const Part::GeomLineSegment*>(geom);
    Base::Vector3d dir = lineSeg->getEndPoint() - lineSeg->getStartPoint();

    // slope = dy / |dx|  (matches GCS ConstraintSlope::error() = dy - s*|dx|)
    if (std::abs(dir.x) < Precision::Confusion()) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("Cannot define a slope for a vertical line."));
        return;
    }
    double slope = dir.y / std::abs(dir.x);

    openCommand(QT_TRANSLATE_NOOP("Command", "Add slope constraint"));
    Gui::cmdAppObjectArgs(Obj,
                          "addConstraint(Sketcher.Constraint('Slope',%d,%f))",
                          GeoId1,
                          slope);

    bool fixed = isPointOrSegmentFixed(Obj, GeoId1);
    if (fixed || constraintCreationMode == Reference) {
        // it is a constraint on an external line, make it non-driving
        const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();
        Gui::cmdAppObjectArgs(Obj,
                              "setDriving(%d,%s)",
                              ConStr.size() - 1,
                              "False");
        finishDatumConstraint(this, Obj, false);
    }
    else {
        finishDatumConstraint(this, Obj, true);
    }
}
```

---

### 4.4 `CmdSketcherConstrainSlope` —— 支持"选择两个点"创建坡度（新增）

**文件**：`src/Mod/Sketcher/Gui/CommandConstraints.cpp`
**位置**：类 `CmdSketcherConstrainSlope`（第 9026 行）

本次为 Slope 命令扩展了 **选择两个点** 的创建方式（除原有"选择一条直线"外）。
选择两个点时，会在两点之间**新增一条构造线（construction line）**，并对其施加 Slope 约束。

**(a) 修改 `allowedSelSequences`**（构造函数内，第 9053 行）

```cpp
    // Either a single line/edge, or two points (a construction line through the
    // two points is added and its slope is constrained).
    allowedSelSequences = {{SelEdge}, {SelVertexOrRoot, SelVertexOrRoot}};
```

**(b) 重写 `applyConstraint`**：在函数开头增加两点分支

```cpp
void CmdSketcherConstrainSlope::applyConstraint(std::vector<SelIdPair>& selSeq, int seqIndex)
{
    Q_UNUSED(seqIndex);

    SketcherGui::ViewProviderSketch* sketchgui =
        static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    if (!sketchgui) {
        return;
    }
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    if (selSeq.size() == 2) {
        // ------------------------------------------------------------------
        // Two points: add a construction line through the two points and
        // constrain the slope of that line.
        // ------------------------------------------------------------------
        const int GeoId1 = selSeq[0].GeoId;
        const Sketcher::PointPos PosId1 = selSeq[0].PosId;
        const int GeoId2 = selSeq[1].GeoId;
        const Sketcher::PointPos PosId2 = selSeq[1].PosId;

        if (!isVertex(GeoId1, PosId1) || !isVertex(GeoId2, PosId2)) {
            Gui::TranslatedUserWarning(
                Obj,
                QObject::tr("Wrong selection"),
                QObject::tr("Select two points from the sketch."));
            return;
        }

        Base::Vector3d pnt1 = Obj->getPoint(GeoId1, PosId1);
        Base::Vector3d pnt2 = Obj->getPoint(GeoId2, PosId2);
        double dx = pnt2.x - pnt1.x;
        double dy = pnt2.y - pnt1.y;

        // slope = dy / |dx|  (matches GCS ConstraintSlope::error() = dy - s*|dx|)
        if (std::abs(dx) < Precision::Confusion()) {
            Gui::TranslatedUserWarning(
                Obj,
                QObject::tr("Wrong selection"),
                QObject::tr("Cannot define a slope between two vertically aligned points."));
            return;
        }
        double slope = dy / std::abs(dx);

        openCommand(QT_TRANSLATE_NOOP("Command", "Add slope constraint"));

        // Add a construction line through the two selected points.
        Gui::cmdAppObjectArgs(Obj,
                              "addGeometry(Part.LineSegment(App.Vector(%f,%f,0),"
                              "App.Vector(%f,%f,0)), True)",
                              pnt1.x,
                              pnt1.y,
                              pnt2.x,
                              pnt2.y);
        int GeoIdNew = Obj->getHighestCurveIndex();

        // Tie the construction line endpoints to the two selected points so that
        // the line (and hence its slope) follows those points.
        Gui::cmdAppObjectArgs(Obj,
                              "addConstraint(Sketcher.Constraint('Coincident',%d,%d,%d,%d))",
                              GeoId1,
                              static_cast<int>(PosId1),
                              GeoIdNew,
                              static_cast<int>(Sketcher::PointPos::start));
        Gui::cmdAppObjectArgs(Obj,
                              "addConstraint(Sketcher.Constraint('Coincident',%d,%d,%d,%d))",
                              GeoId2,
                              static_cast<int>(PosId2),
                              GeoIdNew,
                              static_cast<int>(Sketcher::PointPos::end));

        // Add the slope constraint last so that it is the latest constraint and
        // finishDatumConstraint() will open the dedicated editor for it.
        Gui::cmdAppObjectArgs(Obj,
                              "addConstraint(Sketcher.Constraint('Slope',%d,%f))",
                              GeoIdNew,
                              slope);

        if (constraintCreationMode == Reference) {
            // The new slope is on an internal construction line; only a
            // Reference (non-driving) mode makes it non-driving.
            const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();
            Gui::cmdAppObjectArgs(Obj,
                                  "setDriving(%d,%s)",
                                  ConStr.size() - 1,
                                  "False");
            finishDatumConstraint(this, Obj, false);
        }
        else {
            finishDatumConstraint(this, Obj, true);
        }
        return;
    }

    if (selSeq.size() != 1) {
        return;
    }

    const int GeoId1 = selSeq[0].GeoId;
    const Sketcher::PointPos PosId1 = selSeq[0].PosId;

    if (!isEdge(GeoId1, PosId1)) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("Select a line from the sketch."));
        return;
    }

    if (isBsplinePole(Obj, GeoId1)) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("Select an edge that is not a B-spline weight."));
        return;
    }

    const Part::Geometry* geom = Obj->getGeometry(GeoId1);
    if (!(geom && isLineSegment(*geom))) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("Slope constraint can only be applied to a line segment."));
        return;
    }

    auto lineSeg = static_cast<const Part::GeomLineSegment*>(geom);
    Base::Vector3d dir = lineSeg->getEndPoint() - lineSeg->getStartPoint();

    // slope = dy / |dx|  (matches GCS ConstraintSlope::error() = dy - s*|dx|)
    if (std::abs(dir.x) < Precision::Confusion()) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("Cannot define a slope for a vertical line."));
        return;
    }
    double slope = dir.y / std::abs(dir.x);

    openCommand(QT_TRANSLATE_NOOP("Command", "Add slope constraint"));
    Gui::cmdAppObjectArgs(Obj,
                          "addConstraint(Sketcher.Constraint('Slope',%d,%f))",
                          GeoId1,
                          slope);

    bool fixed = isPointOrSegmentFixed(Obj, GeoId1);
    if (fixed || constraintCreationMode == Reference) {
        // it is a constraint on an external line, make it non-driving
        const std::vector<Sketcher::Constraint*>& ConStr = Obj->Constraints.getValues();
        Gui::cmdAppObjectArgs(Obj,
                              "setDriving(%d,%s)",
                              ConStr.size() - 1,
                              "False");
        finishDatumConstraint(this, Obj, false);
    }
    else {
        finishDatumConstraint(this, Obj, true);
    }
}
```

> 说明：两点方案与单线方案共用 `finishDatumConstraint`，因此两点创建时同样会弹出
> `EditSlopeDialog` 供用户确认/修改坡度值。选择两个垂直对齐的点时会提示错误。

---

### 5. `EditDatumDialog.cpp`（修改）

**文件**：`src/Mod/Sketcher/Gui/EditDatumDialog.cpp`
**位置**：构造函数 `EditDatumDialog` 内，`Constr->Type` 的 `else if` 分支链中（第 163 行）

保留 Slope 分支作为通用对话框的兜底路径（当未走专用对话框时也能正确显示无量纲坡度）：

```cpp
        else if (Constr->Type == Sketcher::Slope) {
            dlg.setWindowTitle(tr("Insert Slope"));
            // Slope is a dimensionless ratio (dy/dx).  Engineering practice
            // expresses slope as "1:n" (e.g. slope = 0.5  =>  1:2).
            // Keep the Quantity unitless so the spin box does not display "mm".
            ui_ins_datum->label->setText(tr("Slope (1:n):"));
            ui_ins_datum->labelEdit->setParamGrpPath(
                QByteArray("User parameter:BaseApp/History/SketcherSlope")
            );
            ui_ins_datum->labelEdit->setSingleStep(0.05);
        }
```

---

### 6. `TaskSketcherConstraints.cpp`（修改）

**文件**：`src/Mod/Sketcher/Gui/TaskSketcherConstraints.cpp`

#### 6.1 约束面板列表显示 `(±1:m)`

**位置**：函数 `TaskSketcherConstraints::updateConstraints` 内，`Constr->Type` 的 `case` 分支中（第 176 行）

内部值 `v = dy/dx`，则 `m = 1/v`；符号放在 `1` 之前（负坡度显示 `-1:m`）：

```cpp
                case Sketcher::Slope: {
                    // Slope is a dimensionless ratio dy/dx.  Per the engineering
                    // convention (https://baike.baidu.com/item/%E8%BE%B9%E5%9D%A1%E5%9D%A1%E5%BA%A6)
                    // a slope is written as "1:n" (e.g. slope = 0.5  =>  1:2).
                    // Internally FreeCAD stores the value as the raw dy/dx ratio
                    // so 1:n in display corresponds to dy/dx = 1/n (run / rise).
                    // The sign of the slope is shown before "1" (e.g. -1:1
                    // rather than 1:-1) so the negative sign is unambiguous.
                    double v = constraint->getPresentationValue().getValue();
                    double n = 1.0;
                    if (fabs(v) > 0.0) {
                        n = fabs(1.0 / v);
                    }
                    const QString sign = (v < 0.0) ? QStringLiteral("-") : QString();
                    const QString slopeStr = sign + QStringLiteral("1:%1").arg(n);
                    name = QStringLiteral("%1 (%2)").arg(name, slopeStr);
                    break;
                }
```

#### 6.2 `getIcon()` 中新增 Slope 图标分支

**位置**：函数 `getIcon` 的 `case` 分支中（第 341 行）。
同时新增 `slope` 图标变量声明（第 257 行，`Constraint_Slope` 主题图标）：

**图标变量声明（第 257 行附近）**

```cpp
            static QIcon slope(Gui::BitmapFactory().iconFromTheme("Constraint_Slope"));
```

**case 分支（第 341 行附近）**

```cpp
                case Sketcher::Slope:
                    // Slope currently has no dedicated Driven variant; reuse the
                    // normal icon (same approach used by Block / Horizontal).
                    return selicon(constraint, slope, slope);
```

#### 6.3 几何引用判断中加入 Slope

**位置**：`constraintHasGeometry`（或等效的几何判断函数）的 `case` 分支中（第 410 行附近）

```cpp
            case Sketcher::Angle:
            case Sketcher::SnellsLaw:
            case Sketcher::Slope:      // 新增
                return (constraint->First >= 0 || constraint->Second >= 0
                        || constraint->Third >= 0);
```

---

### 7. `EditModeConstraintCoinManager.cpp`（修改）

**文件**：`src/Mod/Sketcher/Gui/EditModeConstraintCoinManager.cpp`

#### 7.1 新增 `<cmath>` include

**位置**：第 51 行 `#include <QRegularExpression>` 之后（`std::abs` 用）

```cpp
#include <QPainter>
#include <QRegularExpression>
#include <cmath>      // 新增
#include <limits>
#include <memory>
```

#### 7.2 3D 视图标签显示 `±1:m`

**位置**：函数 `getPresentationString` 内，生成值字符串之后（第 2245 行起）

```cpp
    // Get the current value string including units
    double factor {};
    std::string unitStr;  // the actual unit string
    const auto constrPresValue {constraint->getPresentationValue().getUserString(factor, unitStr)};
    auto valueStr = QString::fromStdString(constrPresValue);

    // A slope is a dimensionless ratio dy/dx.  Per the engineering convention a
    // slope is written as "1:m" (e.g. dy/dx = 0.5  =>  1:2).  Internally FreeCAD
    // stores dy/dx, so "1:m" in display corresponds to m = 1/(dy/dx).  The sign
    // of the slope (uphill vs downhill) is shown before the "1" so that a
    // negative slope reads e.g. "-1:1" rather than "1:-1".
    if (constraint->Type == Sketcher::Slope) {
        double v = constraint->getPresentationValue().getValue();
        double m = 1.0;
        if (std::abs(v) > 1e-12) {
            m = std::abs(1.0 / v);
        }
        const QString sign = (v < 0.0) ? QStringLiteral("-") : QString();
        valueStr = sign + QStringLiteral("1:%1").arg(m);
        unitStr.clear();
    }

    auto fixedValueStr = fixValueStr(valueStr, unitStr).value_or(valueStr);
```

> 注：`unitStr.clear()` 确保标签不显示任何单位（坡度无量纲）。

---

### 8. `ViewProviderSketch.cpp`（修改）

**文件**：`src/Mod/Sketcher/Gui/ViewProviderSketch.cpp`

#### 8.1 新增 include

**位置**：第 70 行 `#include "EditDatumDialog.h"` 之后

```cpp
#include "DrawSketchHandler.h"
#include "EditDatumDialog.h"
#include "EditSlopeDialog.h"      // 新增
#include "EditModeCoinManager.h"
```

#### 8.2 双击已存在的坡度约束时改用专用对话框

**位置**：`onSelectionChanged` 方法内，"if its the right constraint" 处（第 1333 行）

```cpp
            // if its the right constraint
            if (Constr->isDimensional()) {
                Gui::Command::openCommand(
                    QT_TRANSLATE_NOOP("Command", "Modify sketch constraints"));
                if (Constr->Type == Sketcher::Slope) {
                    // Slope is a dimensionless ratio; give it a dedicated
                    // "1:n" editor instead of the generic datum dialog.
                    EditSlopeDialog editSlopeDialog(this, id);
                    editSlopeDialog.exec();
                }
                else {
                    EditDatumDialog editDatumDialog(this, id);
                    editDatumDialog.exec();
                }
            }
```

---

## 五、显示效果汇总

| 内部值 dy/dx | 约束面板 | 3D 视图标签 | 编辑对话框初始值 |
|-------------|----------|-------------|-----------------|
| `0.5` | `(1:2)` | `1:2` | `1:2` |
| `2`   | `(1:0.5)` | `1:0.5` | `1:0.5` |
| `-0.5`| `(-1:2)` | `-1:2` | `-1:2` |
| `-1`  | `(-1:1)` | `-1:1` | `-1:1` |
| `0`   | `(1:1)` | `1:1` | `0`（待编辑） |

> 对话框输入解析兼容 `-1:1` 与 `1:-1` 两种写法，均可正确还原为 `dy/dx = -1`。

---

## 六、重新编译

修改位于 Sketcher 模块，需重新编译并重新运行：

```bash
# 例如在构建目录中
cmake --build <build_dir> --target SketcherGui
```

或直接重新构建并启动 FreeCAD，然后对草图中的坡度约束进行验证。
