/**
 *
 * Copyright (c) 2013 Pascal Gauthier.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 */

#include <time.h>

#include "PluginParam.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"

// ************************************************************************
//
Ctrl::Ctrl(String name) {
    label << name;
    slider = NULL;
    button = NULL;
    comboBox = NULL;
}

void Ctrl::bind(Slider *s) {
    slider = s;
    updateComponent();
    s->addListener(this);
}

void Ctrl::bind(Button *b) {
    button = b;
    updateComponent();
    b->addListener(this);
}

void Ctrl::bind(ComboBox *c) {
    comboBox = c;
    updateComponent();
    c->addListener(this);
}

void Ctrl::unbind() {
    if (slider != NULL) {
        slider->removeListener(this);
        slider = NULL;
    }

    if (button != NULL) {
        button->removeListener(this);
        button = NULL;
    }

    if (comboBox != NULL) {
        comboBox->removeListener(this);
        comboBox = NULL;
    }
}

void Ctrl::publishValue(float value) {
    parent->beginParameterChangeGesture(idx);
    parent->setParameterNotifyingHost(idx, value);
    parent->endParameterChangeGesture(idx);
}

void Ctrl::sliderValueChanged(Slider* moved) {
    publishValue(moved->getValue());
}

void Ctrl::buttonClicked(Button* clicked) {
    publishValue(clicked->getToggleStateValue() == 1 ? 1 : 0);
}

void Ctrl::comboBoxChanged(ComboBox* combo) {
    publishValue((combo->getSelectedId() - 1) / combo->getNumItems());
}

// ************************************************************************
// CtrlDX - control DX mapping
CtrlFloat::CtrlFloat(String name, float *storageValue) : Ctrl(name) {
    vPointer = storageValue;
}

float CtrlFloat::getValueHost() {
    return *vPointer;
}

void CtrlFloat::setValueHost(float v) {
    *vPointer = v;
}

String CtrlFloat::getValueDisplay() {
    String display;
    display << *vPointer;
    return display;
}

void CtrlFloat::updateComponent() {
    if (slider != NULL) {
        slider->setValue(*vPointer, dontSendNotification);
    }
}

// ************************************************************************
// CtrlDX - control DX mapping
CtrlDX::CtrlDX(String name, int steps, int offset, int displayValue) : Ctrl(name) {
    this->displayValue = displayValue;
    this->steps = steps;
    dxValue = 0;
    dxOffset = offset;
}

float CtrlDX::getValueHost() {
    return dxValue / steps;
}

void CtrlDX::setValueHost(float f) {
    setValue((f * steps));
}

void CtrlDX::setValue(int v) {
    TRACE("setting value %d %d", dxOffset, v);
    if (v >= steps) {
        TRACE("WARNING: value too big %s : %d", label.toRawUTF8(), v);
        v = steps - 1;
    }
    dxValue = v;
    if (dxOffset >= 0) {
        if (parent != NULL)
            parent->setDxValue(dxOffset, dxValue);
    }
}

int CtrlDX::getValue() {
    if (dxOffset >= 0)
        dxValue = parent->data[dxOffset];
    return dxValue;
}

int CtrlDX::getOffset() {
    return dxOffset;
}

String CtrlDX::getValueDisplay() {
    String ret;
    ret << ( getValue() + displayValue );
    return ret;
}

void CtrlDX::publishValue(float value) {
    Ctrl::publishValue(value / steps);

    DexedAudioProcessorEditor *editor = (DexedAudioProcessorEditor *) parent->getActiveEditor();
    if ( editor == NULL )
        return;
    String msg;
    msg << label << " = " << getValueDisplay();
    editor->global.setParamMessage(msg);
}

void CtrlDX::sliderValueChanged(Slider* moved) {
    publishValue(((int) moved->getValue() - displayValue));
}

void CtrlDX::comboBoxChanged(ComboBox* combo) {
    publishValue(combo->getSelectedId() - 1);
}

void CtrlDX::updateComponent() {
    if (slider != NULL) {
        slider->setValue(getValue() + displayValue,
                dontSendNotification);
    }

    if (button != NULL) {
        if (getValue() == 0) {
            button->setToggleState(false,
                    dontSendNotification);
        } else {
            button->setToggleState(true,
                    dontSendNotification);
        }
    }

    if (comboBox != NULL) {
        int cvalue = getValue() + 1;
        if (comboBox->getNumItems() <= cvalue) {
            cvalue = comboBox->getNumItems();
        }
        comboBox->setSelectedId(cvalue, dontSendNotification);
    }
}

/***************************************************************
 *
 */
void DexedAudioProcessor::initCtrl() {
    loadBuiltin(0);
    currentProgram = 0;
    
    fxCutoff = new CtrlFloat("Cutoff", &fx.uiCutoff);
    ctrl.add(fxCutoff);
    
    fxReso = new CtrlFloat("Resonance", &fx.uiReso);
    ctrl.add(fxReso);
    
    output = new CtrlFloat("Output", &fx.uiGain);
    ctrl.add(output);
    
    algo = new CtrlDX("ALGORITHM", 32, 134, 1);
    ctrl.add(algo);
    
    feedback = new CtrlDX("FEEDBACK", 8, 135);
    ctrl.add(feedback);
    
    oscSync = new CtrlDX("OSC KEY SYNC", 2, 136);
    ctrl.add(oscSync);
    
    lfoRate = new CtrlDX("LFO SPEED", 100, 137);
    ctrl.add(lfoRate);
    
    lfoDelay = new CtrlDX("LFO DELAY", 100, 138);
    ctrl.add(lfoDelay);
    
    lfoPitchDepth = new CtrlDX("LFO PM DEPTH", 100, 139);
    ctrl.add(lfoPitchDepth);
    
    lfoAmpDepth = new CtrlDX("LFO AM DEPTH", 100, 140);
    ctrl.add(lfoAmpDepth);
    
    lfoSync = new CtrlDX("LFO KEY SYNC", 2, 141);
    ctrl.add(lfoSync);
    
    lfoWaveform = new CtrlDX("LFO WAVE", 5, 142);
    ctrl.add(lfoWaveform);
    
    transpose = new CtrlDX("MIDDLE C", 49, 144);
    ctrl.add(transpose);
    
    pitchModSens = new CtrlDX("P MODE SENS.", 8, 143);
    ctrl.add(pitchModSens);
    
    for (int i=0;i<4;i++) {
        String rate;
        rate << "PITCH EG RATE " << (i+1);
        pitchEgRate[i] = new CtrlDX(rate, 99, 126+i);
        ctrl.add(pitchEgRate[i]);
    }

    for (int i=0;i<4;i++) {
        String level;
        level << "PITCH EG LEVEL " << (i+1);
        pitchEgLevel[i] = new CtrlDX(level, 99, 130+i);
        ctrl.add(pitchEgLevel[i]);
    }
    
    // fill operator values;
    for (int i = 0; i < 6; i++) {
        //// In the Sysex, OP6 comes first, then OP5...
        int opTarget = (5-i) * 21;
        int opVal = i;
        String opName;
        opName << "OP" << (opVal + 1);

        for (int j = 0; j < 4; j++) {     
            String opRate;
            opRate << opName << " EG RATE " << (j + 1);
            opCtrl[opVal].egRate[j] = new CtrlDX(opRate, 100, opTarget + j);
            ctrl.add(opCtrl[opVal].egRate[j]);
        }
    
        for (int j = 0; j < 4; j++) {        
            String opLevel;
            opLevel << opName << " EG LEVEL " << (j + 1);
            opCtrl[opVal].egLevel[j] = new CtrlDX(opLevel, 100, opTarget + j + 4);
            ctrl.add(opCtrl[opVal].egLevel[j]);
        }
    
        String opVol;
        opVol << opName << " OUTPUT LEVEL";
        opCtrl[opVal].level = new CtrlDX(opVol, 100, opTarget + 16);
        ctrl.add(opCtrl[opVal].level);

        String opMode;
        opMode << opName << " MODE";
        opCtrl[opVal].opMode = new CtrlDX(opMode, 2, opTarget + 17);
        ctrl.add(opCtrl[opVal].opMode);

        String coarse;
        coarse << opName << " F COARSE";
        opCtrl[opVal].coarse = new CtrlDX(coarse, 32, opTarget + 18);
        ctrl.add(opCtrl[opVal].coarse);

        String fine;
        fine << opName << " F FINE";
        opCtrl[opVal].fine = new CtrlDX(fine, 100, opTarget + 19);
        ctrl.add(opCtrl[opVal].fine);

        String detune;
        detune << opName << " OSC DETUNE";
        opCtrl[opVal].detune = new CtrlDX(detune, 15, opTarget + 20, -7);
        ctrl.add(opCtrl[opVal].detune);

        String sclBrkPt;
        sclBrkPt << opName << " BREAK POINT";
        opCtrl[opVal].sclBrkPt = new CtrlDX(sclBrkPt, 100, opTarget + 8);
        ctrl.add(opCtrl[opVal].sclBrkPt);

        String sclLeftDepth;
        sclLeftDepth << opName << " L SCALE DEPTH";
        opCtrl[opVal].sclLeftDepth = new CtrlDX(sclLeftDepth, 100, opTarget + 9);
        ctrl.add(opCtrl[opVal].sclLeftDepth);

        String sclRightDepth;
        sclRightDepth << opName << " R SCALE DEPTH";
        opCtrl[opVal].sclRightDepth = new CtrlDX(sclRightDepth, 100, opTarget + 10);
        ctrl.add(opCtrl[opVal].sclRightDepth);

        String sclLeftCurve;
        sclLeftCurve << opName << " L KEY SCALE";
        opCtrl[opVal].sclLeftCurve = new CtrlDX(sclLeftCurve, 4, opTarget + 11);
        ctrl.add(opCtrl[opVal].sclLeftCurve);

        String sclRightCurve;
        sclRightCurve << opName << " R KEY SCALE";
        opCtrl[opVal].sclRightCurve = new CtrlDX(sclRightCurve, 4, opTarget + 12);
        ctrl.add(opCtrl[opVal].sclRightCurve);

        String sclRate;
        sclRate << opName << " RATE SCALING";
        opCtrl[opVal].sclRate = new CtrlDX(sclRate, 8, opTarget + 13);
        ctrl.add(opCtrl[opVal].sclRate);

        String ampModSens;
        ampModSens << opName << " A MOD SENS.";
        opCtrl[opVal].ampModSens = new CtrlDX(ampModSens, 4, opTarget + 14);
        ctrl.add(opCtrl[opVal].ampModSens);

        String velModSens;
        velModSens << opName << " KEY VELOCITY";
        opCtrl[opVal].velModSens = new CtrlDX(velModSens, 8, opTarget + 15);
        ctrl.add(opCtrl[opVal].velModSens);
    }
    
    for (int i=0; i < ctrl.size(); i++) {
        ctrl[i]->idx = i;
        ctrl[i]->parent = this;
    }
}

void DexedAudioProcessor::setDxValue(int offset, int v) {
    TRACE("setting dx %d %d", offset, v);
    refreshVoice = true;
    if (offset >= 0)
        data[offset] = v;

    if (!sendSysexChange)
        return;
    uint8 msg[7] = { 0xF0, 0x43, 0x10, offset > 127, 0, (uint8) v, 0xF7 };
    msg[4] = offset & 0x7F;
    midiOut.addEvent(msg, 7, 0);
}

void DexedAudioProcessor::unbindUI() {
    for (int i = 0; i < ctrl.size(); i++) {
        ctrl[i]->unbind();
    }
}

//==============================================================================
int DexedAudioProcessor::getNumParameters() {
    return ctrl.size();
}

float DexedAudioProcessor::getParameter(int index) {
    return ctrl[index]->getValueHost();
}

void DexedAudioProcessor::setParameter(int index, float newValue) {
    ctrl[index]->setValueHost(newValue);
    forceRefreshUI = true;
}

int DexedAudioProcessor::getNumPrograms() {
    return 32;
}

int DexedAudioProcessor::getCurrentProgram() {
    return currentProgram;
}

void DexedAudioProcessor::setCurrentProgram(int index) {
    TRACE("setting program %d state", index);

    if ( lastStateSave + 2 > time(NULL) ) {
        TRACE("skipping save, storage recall to close");
        return;
    }
    
    for (int i = 0; i < MAX_ACTIVE_NOTES; i++) {
        if (voices[i].keydown == false && voices[i].live == true) {
            voices[i].live = false;
        }
    }
    index = index > 31 ? 31 : index;
    unpackProgram(index);
    lfo.reset(data + 137);
    currentProgram = index;
    triggerAsyncUpdate();
}

const String DexedAudioProcessor::getProgramName(int index) {
    if (index >= 32)
        index = 31;
    return programNames[index];
}

void DexedAudioProcessor::changeProgramName(int index, const String& newName) {
}

const String DexedAudioProcessor::getParameterName(int index) {
    return ctrl[index]->label;
}

const String DexedAudioProcessor::getParameterText(int index) {
    return ctrl[index]->getValueDisplay();
}

void DexedAudioProcessor::loadPreference() {
    PropertiesFile prop(prefOptions);
    
    if ( ! prop.isValidFile() ) {
        return;
    }
    
    if ( prop.containsKey( String("normalizeDxVelocity") ) ) {
        normalizeDxVelocity = prop.getIntValue( String("normalizeDxVelocity") );
    }
    
    if ( prop.containsKey( String("pitchRange") ) ) {
        controllers.values_[kControllerPitchRange] = prop.getIntValue( String("pitchRange") );
    }
    
    if ( prop.containsKey( String("pitchStep") ) ) {
        controllers.values_[kControllerPitchStep] = prop.getIntValue( String("pitchStep") );
    }
    
}

void DexedAudioProcessor::savePreference() {
    PropertiesFile prop(prefOptions);
    
    prop.setValue(String("normalizeDxVelocity"), normalizeDxVelocity);
    prop.setValue(String("pitchRange"), controllers.values_[kControllerPitchRange]);
    prop.setValue(String("pitchStep"), controllers.values_[kControllerPitchStep]);
    
    prop.save();
}

