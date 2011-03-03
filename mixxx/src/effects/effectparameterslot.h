#ifndef EFFECTPARAMETERSLOT_H
#define EFFECTPARAMETERSLOT_H

#include <QObject>
#include <QMutex>
#include <QVariant>
#include <QString>

#include "util.h"
#include "controlobject.h"
#include "effects/effect.h"

class EffectParameterSlot : QObject {
    Q_OBJECT
  public:
    EffectParameterSlot(QObject* pParent, QString group, unsigned int parameterNumber);
    virtual ~EffectParameterSlot();

    // Load the parameter of the given effect into this EffectParameterSlot
    void loadEffect(EffectPointer pEffect);

    // bool isEnabled() const;

    // double getValue() const;
    // void setValue(double value);

    // double getValueNormalized() const;
    // void setValueNormalized(double value);

    // double getDefault() const;
    // void setDefault(double defaultValue);

    // double getMinimum() const;
    // void setMinimum(double minimum);

    // double getMaximum() const;
    // void setMaximum(double maximum);

    // double getMaximumLimit() const;
    // double getMinimumLimit() const;

  private slots:
    // Solely for handling control changes
    void slotEnabled(double v);
    void slotValue(double v);
    void slotValueNormalized(double v);
    void slotValueType(double v);
    void slotValueDefault(double v);
    void slotValueMaximum(double v);
    void slotValueMaximumLimit(double v);
    void slotValueMinimum(double v);
    void slotValueMinimumLimit(double v);

  private:
    DISALLOW_COPY_AND_ASSIGN(EffectParameterSlot);
    QString debugString() const {
        return QString("EffectParameterSlot(%1,%2)").arg(m_group).arg(m_iParameterNumber);
    }

    // Clear the currently loaded effect
    void clear();

    mutable QMutex m_mutex;
    const unsigned int m_iParameterNumber;
    QString m_group;
    EffectPointer m_pEffect;

    ////////////////////////////////////////////////////////////////////////////////
    // Controls exposed to the rest of Mixxx
    ////////////////////////////////////////////////////////////////////////////////

    ControlObject* m_pControlEnabled;
    ControlObject* m_pControlValue;
    ControlObject* m_pControlValueNormalized;
    ControlObject* m_pControlValueType;
    ControlObject* m_pControlValueDefault;
    ControlObject* m_pControlValueMaximum;
    ControlObject* m_pControlValueMaximumLimit;
    ControlObject* m_pControlValueMinimum;
    ControlObject* m_pControlValueMinimumLimit;
};

#endif /* EFFECTPARAMETERSLOT_H */
