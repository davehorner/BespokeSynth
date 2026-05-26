/**
    bespoke synth, a software modular synthesizer
    Copyright (C) 2021 Ryan Challinor (contact: awwbees@gmail.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once

#include "ClickButton.h"
#include "DropdownList.h"
#include "IDrawableModule.h"
#include "INoteReceiver.h"
#include "IPulseReceiver.h"
#include "Slider.h"
#include "TextEntry.h"

#include "juce_osc/juce_osc.h"

#include <array>
#include <string>
#include <vector>

struct CuneusInstance;

class Cuneus : public IDrawableModule, public IFloatSliderListener, public IIntSliderListener, public IDropdownListener, public IButtonListener, public ITextEntryListener, public IPulseReceiver, public INoteReceiver, private juce::OSCReceiver, private juce::OSCReceiver::Listener<juce::OSCReceiver::MessageLoopCallback>
{
public:
   Cuneus();
   ~Cuneus() override;
   static IDrawableModule* Create() { return new Cuneus(); }
   static bool AcceptsAudio() { return false; }
   static bool AcceptsNotes() { return true; }
   static bool AcceptsPulses() { return true; }

   void CreateUIControls() override;
   void Init() override;
   void Poll() override;

   void SetEnabled(bool enabled) override { mEnabled = enabled; }
   bool IsEnabled() const override { return mEnabled; }

   void PlayNote(NoteMessage note) override;
   void SendCC(int control, int value, int voiceIdx = -1) override {}
   void OnPulse(double time, float velocity, int flags) override;

   void FloatSliderUpdated(FloatSlider* slider, float oldVal, double time) override;
   void IntSliderUpdated(IntSlider* slider, int oldVal, double time) override;
   void DropdownUpdated(DropdownList* list, int oldVal, double time) override;
   void ButtonClicked(ClickButton* button, double time) override;
   void TextEntryComplete(TextEntry* entry) override;
   void oscMessageReceived(const juce::OSCMessage& msg) override;

   void LoadLayout(const ofxJSONElement& moduleInfo) override;
   void SaveLayout(ofxJSONElement& moduleInfo) override;
   void SetUpFromSaveData() override;
   void SaveState(FileStreamOut& out) override;
   void LoadState(FileStreamIn& in, int rev) override;
   int GetModuleSaveStateRev() const override { return 1; }
   std::vector<IUIControl*> ControlsToIgnoreInSaveState() const override;

private:
   struct ParamControl
   {
      std::string id;
      int type{ 0 };
      std::array<float, 3> values{ 0, 0, 0 };
      std::array<FloatSlider*, 3> sliders{ nullptr, nullptr, nullptr };
   };

   void DrawModule() override;
   void OpenInstance();
   void CloseInstance();
   void RefreshBinList();
   void RefreshParamControls();
   void ClearParamControls();
   void SendParam(ParamControl& param);
   bool StartFeedbackReceiver();
   void StopFeedbackReceiver();
   void RequestDiscovery();
   void ApplyFeedbackValue(const std::string& id, const juce::OSCMessage& msg);
   std::string GetSelectedBinName() const;
   std::string GetDefaultExecutableDir() const;
   void SetStatus(std::string status);

   CuneusInstance* mInstance{ nullptr };
   std::vector<std::string> mBinNames;
   std::vector<ParamControl> mParams;
   std::string mExecutableDir;
   std::string mStatus;
   int mSelectedBin{ 0 };
   int mRemotePort{ 7841 };
   int mFeedbackPort{ 7842 };
   bool mFeedbackReceiverConnected{ false };
   bool mApplyingFeedback{ false };
   int mPendingDiscoveryRequests{ 0 };
   double mLastDiscoveryRequestTime{ -9999 };

   DropdownList* mBinDropdown{ nullptr };
   TextEntry* mExecutableDirEntry{ nullptr };
   IntSlider* mPortSlider{ nullptr };
   ClickButton* mOpenButton{ nullptr };
   ClickButton* mCloseButton{ nullptr };
};
