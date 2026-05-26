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

#include "Cuneus.h"

#include "FileStream.h"
#include "SynthGlobals.h"
#include "UIControlMacros.h"
#include "ofxJSONElement.h"

#if BESPOKE_CUNEUS_ENABLED
#ifdef __cplusplus
extern "C" {
#endif
#include "cuneus_capi.h"
#ifdef __cplusplus
}
#endif
#endif

#include "juce_core/juce_core.h"

#include <algorithm>

namespace
{
   constexpr int kParamStartY = 76;
   constexpr int kSliderWidth = 150;

#if !BESPOKE_CUNEUS_ENABLED
   enum CuneusParamType
   {
      CUNEUS_PARAM_F32 = 0,
      CUNEUS_PARAM_COLOR3 = 1
   };
#endif
}

Cuneus::Cuneus()
: IDrawableModule(330, 100)
{
   mExecutableDir = GetDefaultExecutableDir();
}

Cuneus::~Cuneus()
{
   CloseInstance();
   ClearParamControls();
}

void Cuneus::Init()
{
   IDrawableModule::Init();
}

void Cuneus::CreateUIControls()
{
   IDrawableModule::CreateUIControls();

   UIBLOCK0();
   DROPDOWN(mBinDropdown, "shader", &mSelectedBin, 120);
   mBinDropdown->DrawLabel(true);
   UIBLOCK_SHIFTRIGHT();
   BUTTON(mOpenButton, "open");
   UIBLOCK_SHIFTRIGHT();
   BUTTON(mCloseButton, "close");
   UIBLOCK_NEWLINE();
   TEXTENTRY(mExecutableDirEntry, "exe dir", 48, &mExecutableDir);
   mExecutableDirEntry->DrawLabel(true);
   UIBLOCK_NEWLINE();
   INTSLIDER(mPortSlider, "port", &mRemotePort, 1024, 65535);
   ENDUIBLOCK0();

   RefreshBinList();
}

void Cuneus::Poll()
{
}

void Cuneus::RefreshBinList()
{
   if (mBinDropdown == nullptr)
      return;

   mBinNames.clear();
   mBinDropdown->Clear();

#if BESPOKE_CUNEUS_ENABLED
   const size_t binCount = cuneus_bin_count();
   for (size_t i = 0; i < binCount; ++i)
   {
      const char* name = cuneus_bin_name(i);
      if (name != nullptr)
      {
         mBinNames.push_back(name);
         mBinDropdown->AddLabel(name, (int)i);
      }
   }
#endif

   if (mBinNames.empty())
   {
      mBinNames.push_back("roto");
      mBinDropdown->AddLabel("roto", 0);
   }

   mSelectedBin = std::clamp(mSelectedBin, 0, (int)mBinNames.size() - 1);
}

std::string Cuneus::GetSelectedBinName() const
{
   if (mSelectedBin >= 0 && mSelectedBin < (int)mBinNames.size())
      return mBinNames[mSelectedBin];
   return "roto";
}

std::string Cuneus::GetDefaultExecutableDir() const
{
   return juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory().getFullPathName().toStdString();
}

void Cuneus::OpenInstance()
{
#if BESPOKE_CUNEUS_ENABLED
   CloseInstance();
   const std::string binName = GetSelectedBinName();
   mInstance = cuneus_instance_open(binName.c_str(), mExecutableDir.c_str(), (uint16_t)mRemotePort);
   if (mInstance == nullptr)
   {
      const char* error = cuneus_last_error();
      SetStatus(error != nullptr && error[0] != '\0' ? error : "failed to open cuneus");
      return;
   }

   RefreshParamControls();
   SetStatus("opened " + binName);
#else
   SetStatus("BespokeSynth was built without cuneus support");
#endif
}

void Cuneus::CloseInstance()
{
#if BESPOKE_CUNEUS_ENABLED
   if (mInstance != nullptr)
   {
      cuneus_instance_free(mInstance);
      mInstance = nullptr;
   }
#else
   mInstance = nullptr;
#endif
   ClearParamControls();
}

void Cuneus::ClearParamControls()
{
   for (auto& param : mParams)
   {
      for (auto* slider : param.sliders)
      {
         if (slider != nullptr)
            RemoveUIControl(slider);
      }
   }
   mParams.clear();
   mHeight = 100;
}

void Cuneus::RefreshParamControls()
{
   ClearParamControls();

#if BESPOKE_CUNEUS_ENABLED
   if (mInstance == nullptr)
      return;

   const size_t count = cuneus_param_count(mInstance);
   mParams.reserve(count);
   int y = kParamStartY;
   for (size_t i = 0; i < count; ++i)
   {
      CuneusParamDesc desc{};
      if (cuneus_param_desc(mInstance, i, &desc) != CUNEUS_STATUS_OK || desc.id == nullptr)
         continue;

      mParams.emplace_back();
      ParamControl& param = mParams.back();
      param.id = desc.id;
      param.type = (int)desc.param_type;
      param.values[0] = desc.default_value;
      param.values[1] = desc.default_value;
      param.values[2] = desc.default_value;

      if (desc.param_type == CUNEUS_PARAM_COLOR3)
      {
         param.sliders[0] = new FloatSlider(this, (param.id + " r").c_str(), 5, y, kSliderWidth, 15, &param.values[0], desc.min_value, desc.max_value);
         param.sliders[1] = new FloatSlider(this, (param.id + " g").c_str(), 5, y + 16, kSliderWidth, 15, &param.values[1], desc.min_value, desc.max_value);
         param.sliders[2] = new FloatSlider(this, (param.id + " b").c_str(), 5, y + 32, kSliderWidth, 15, &param.values[2], desc.min_value, desc.max_value);
         y += 50;
      }
      else
      {
         param.sliders[0] = new FloatSlider(this, param.id.c_str(), 5, y, kSliderWidth, 15, &param.values[0], desc.min_value, desc.max_value);
         y += 18;
      }

      SendParam(param);
   }

   mHeight = std::max(100, y + 24);
#endif
}

void Cuneus::SendParam(ParamControl& param)
{
#if BESPOKE_CUNEUS_ENABLED
   if (mInstance == nullptr)
      return;

   if (param.type == CUNEUS_PARAM_COLOR3)
      cuneus_set_param_color3(mInstance, param.id.c_str(), param.values[0], param.values[1], param.values[2]);
   else
      cuneus_set_param_f32(mInstance, param.id.c_str(), param.values[0]);
#endif
}

void Cuneus::FloatSliderUpdated(FloatSlider* slider, float oldVal, double time)
{
   for (auto& param : mParams)
   {
      for (auto* paramSlider : param.sliders)
      {
         if (slider == paramSlider)
         {
            SendParam(param);
            return;
         }
      }
   }
}

void Cuneus::IntSliderUpdated(IntSlider* slider, int oldVal, double time)
{
}

void Cuneus::DropdownUpdated(DropdownList* list, int oldVal, double time)
{
}

void Cuneus::ButtonClicked(ClickButton* button, double time)
{
   if (button == mOpenButton)
      OpenInstance();
   if (button == mCloseButton)
   {
      CloseInstance();
      SetStatus("closed");
   }
}

void Cuneus::TextEntryComplete(TextEntry* entry)
{
}

void Cuneus::PlayNote(NoteMessage note)
{
#if BESPOKE_CUNEUS_ENABLED
   if (mInstance != nullptr)
      cuneus_note(mInstance, note.pitch, note.velocity / 127.0f);
#endif
}

void Cuneus::OnPulse(double time, float velocity, int flags)
{
#if BESPOKE_CUNEUS_ENABLED
   if (mInstance != nullptr)
      cuneus_pulse(mInstance, velocity);
#endif
}

void Cuneus::SetStatus(std::string status)
{
   mStatus = status;
}

void Cuneus::DrawModule()
{
   if (Minimized() || IsVisible() == false)
      return;

   mBinDropdown->Draw();
   mOpenButton->Draw();
   mCloseButton->Draw();
   mExecutableDirEntry->Draw();
   mPortSlider->Draw();

   for (auto& param : mParams)
   {
      for (auto* slider : param.sliders)
      {
         if (slider != nullptr)
            slider->Draw();
      }
   }

   ofPushStyle();
   ofSetColor(255, 255, 255, 60);
   ofRect(5, mHeight - 20, mWidth - 10, 15);
   ofSetColor(40, 40, 40);
   DrawTextNormal(mStatus.empty() ? "ready" : mStatus, 8, mHeight - 8, 8);
   ofPopStyle();
}

void Cuneus::LoadLayout(const ofxJSONElement& moduleInfo)
{
   mModuleSaveData.LoadString("executable_dir", moduleInfo, GetDefaultExecutableDir());
   mModuleSaveData.LoadInt("remote_port", moduleInfo, 7841, 1024, 65535);
   mModuleSaveData.LoadInt("selected_bin", moduleInfo, 0, 0, 1024);
   SetUpFromSaveData();
}

void Cuneus::SaveLayout(ofxJSONElement& moduleInfo)
{
   moduleInfo["executable_dir"] = mExecutableDir;
   moduleInfo["remote_port"] = mRemotePort;
   moduleInfo["selected_bin"] = mSelectedBin;
}

void Cuneus::SetUpFromSaveData()
{
   mExecutableDir = mModuleSaveData.GetString("executable_dir");
   mRemotePort = mModuleSaveData.GetInt("remote_port");
   mSelectedBin = mModuleSaveData.GetInt("selected_bin");

   if (mExecutableDirEntry != nullptr)
   {
      mExecutableDirEntry->SetText(mExecutableDir);
      mExecutableDirEntry->UpdateDisplayString();
   }
   RefreshBinList();
}

void Cuneus::SaveState(FileStreamOut& out)
{
   IDrawableModule::SaveState(out);
   out << mExecutableDir;
   out << mRemotePort;
   out << mSelectedBin;
}

void Cuneus::LoadState(FileStreamIn& in, int rev)
{
   IDrawableModule::LoadState(in, rev);
   if (rev >= 1)
   {
      in >> mExecutableDir;
      in >> mRemotePort;
      in >> mSelectedBin;
   }
   SetUpFromSaveData();
}

std::vector<IUIControl*> Cuneus::ControlsToIgnoreInSaveState() const
{
   std::vector<IUIControl*> ignore;
   ignore.push_back(mOpenButton);
   ignore.push_back(mCloseButton);
   return ignore;
}
