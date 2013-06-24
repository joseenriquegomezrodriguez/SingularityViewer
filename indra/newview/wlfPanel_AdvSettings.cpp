/* Copyright (c) 2010
 *
 * Modular Systems All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *   3. Neither the name Modular Systems nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MODULAR SYSTEMS AND CONTRIBUTORS AS IS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MODULAR SYSTEMS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "llviewerprecompiledheaders.h"

#include "wlfPanel_AdvSettings.h"

#include "llbutton.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "lliconctrl.h"
#include "lloverlaybar.h"
#include "lltextbox.h"
#include "llcombobox.h"
#include "llsliderctrl.h"
#include "llcheckboxctrl.h"
#include "llstartup.h"

#include "llfloaterwindlight.h"
#include "llfloaterwater.h"

#include "lldaycyclemanager.h"
#include "llenvmanager.h"
#include "llwaterparammanager.h"
#include "llwlparamset.h"
#include "llwlparammanager.h"

wlfPanel_AdvSettings::wlfPanel_AdvSettings() : mExpanded(false)
{
	setVisible(false);
	setIsChrome(TRUE);
	setFocusRoot(TRUE);
	if(rlv_handler_t::isEnabled())
		gRlvHandler.setBehaviourToggleCallback(boost::bind(&wlfPanel_AdvSettings::onRlvBehaviorChange, this, _1, _2));
}

//static
void wlfPanel_AdvSettings::updateClass()
{
	if(!wlfPanel_AdvSettings::instanceExists())
		return;
	wlfPanel_AdvSettings::getInstance()->build(); 
}

void wlfPanel_AdvSettings::build()
{
	deleteAllChildren();
	std::string ButtonState;
	if (gSavedSettings.getBOOL("wlfAdvSettingsPopup"))
	{
		mExpanded = true;
		LLUICtrlFactory::getInstance()->buildPanel(this, "wlfPanel_AdvSettings_expanded.xml", &getFactoryMap());
		ButtonState = "arrow_down.tga";
	}
	else
	{
		mExpanded = false;
		LLUICtrlFactory::getInstance()->buildPanel(this, "wlfPanel_AdvSettings.xml", &getFactoryMap());
		ButtonState = "arrow_up.tga";
	}
	getChild<LLButton>("expand")->setImageOverlay(ButtonState);
}

// [RLVa:KB] - Checked: 2013-06-20
void wlfPanel_AdvSettings::updateRlvVisibility()
{
	if(!mExpanded || !rlv_handler_t::isEnabled())
		return;

	bool enable = !gRlvHandler.hasBehaviour(RLV_BHVR_SETENV);
	childSetEnabled("use_estate_wl", enable);
	childSetEnabled("EnvAdvancedWaterButton", enable);
	childSetEnabled("WLWaterPresetsCombo", enable);
	childSetEnabled("WWprev", enable);
	childSetEnabled("WWnext", enable);
	childSetEnabled("EnvAdvancedSkyButton", enable);
	childSetEnabled("WLSkyPresetsCombo", enable);
	childSetEnabled("WLprev", enable);
	childSetEnabled("WLnext", enable);
	childSetEnabled("EnvTimeSlider", enable);
}

void wlfPanel_AdvSettings::onRlvBehaviorChange(ERlvBehaviour eBhvr, ERlvParamType eType)
{
	if(eBhvr == RLV_BHVR_SETENV)
		updateRlvVisibility();
}
// [/RLVa:KB]

void wlfPanel_AdvSettings::refreshLists()
{
	LLEnvManagerNew& env_mgr = LLEnvManagerNew::instance();

	// Populate the combo boxes with appropriate lists of available presets.
	//actually, dont do this, its laggy and not needed to just refresh the selections
	// populateWaterPresetsList();
	// populateSkyPresetsList();
	// populateDayCyclePresetsList();

	//populate the combos with "Default" if using region settings
	if (gSavedSettings.getBOOL("UseEnvironmentFromRegion"))
	{
		mWaterPresetCombo->selectByValue("Default");
		mSkyPresetCombo->selectByValue("Default");
		//mDayCyclePresetCombo->selectByValue("Default");
	}
	else
	{
		// Select the current presets in combo boxes.
		mWaterPresetCombo->selectByValue(env_mgr.getWaterPresetName());
		mSkyPresetCombo->selectByValue(env_mgr.getSkyPresetName());
		//mDayCyclePresetCombo->selectByValue(env_mgr.getDayCycleName());
	}
	
	updateTimeSlider();
}

BOOL wlfPanel_AdvSettings::postBuild()
{
	setVisible(true);
	childSetAction("expand", onClickExpandBtn, this);

	if (mExpanded)
	{
		getChild<LLCheckBoxCtrl>("use_estate_wl")->setCommitCallback(onUseRegionSettings);

		mWaterPresetCombo = getChild<LLComboBox>("WLWaterPresetsCombo");
		mWaterPresetCombo->setCommitCallback(onChangeWWPresetName);

		mSkyPresetCombo = getChild<LLComboBox>("WLSkyPresetsCombo");
		mSkyPresetCombo->setCommitCallback(onChangeWLPresetName);

		// mDayCyclePresetCombo = getChild<LLComboBox>("DCPresetsCombo");
		// mDayCyclePresetCombo->setCommitCallback(onChangeDCPresetName);

		LLEnvManagerNew::instance().setPreferencesChangeCallback(boost::bind(&wlfPanel_AdvSettings::refreshLists, this));
		LLWaterParamManager::getInstance()->setPresetListChangeCallback(boost::bind(&wlfPanel_AdvSettings::populateWaterPresetsList, this));
		LLWLParamManager::getInstance()->setPresetListChangeCallback(boost::bind(&wlfPanel_AdvSettings::populateSkyPresetsList, this));
		// LLDayCycleManager::instance().setModifyCallback(boost::bind(&wlfPanel_AdvSettings::populateDayCyclePresetsList, this));

		populateWaterPresetsList();
		populateSkyPresetsList();
		//populateDayCyclePresetsList();

		// next/prev buttons
		childSetAction("WWnext", onClickWWNext, this);
		childSetAction("WWprev", onClickWWPrev, this);
		childSetAction("WLnext", onClickWLNext, this);
		childSetAction("WLprev", onClickWLPrev, this);

		childSetAction("EnvAdvancedSkyButton", onOpenAdvancedSky, NULL);
		childSetAction("EnvAdvancedWaterButton", onOpenAdvancedWater, NULL);

		mTimeSlider = getChild<LLSliderCtrl>("EnvTimeSlider");
		mTimeSlider->setCommitCallback(onChangeDayTime);
		updateTimeSlider();
		updateRlvVisibility();
	}
	return TRUE;
}

void wlfPanel_AdvSettings::draw()
{
	refresh();
	LLPanel::draw();
}

wlfPanel_AdvSettings::~wlfPanel_AdvSettings ()
{
}

void wlfPanel_AdvSettings::onClickExpandBtn(void* user_data)
{
	gSavedSettings.setBOOL("wlfAdvSettingsPopup",!gSavedSettings.getBOOL("wlfAdvSettingsPopup"));
}

void wlfPanel_AdvSettings::onUseRegionSettings(LLUICtrl* ctrl, void* userdata)
{
	LLEnvManagerNew::instance().setUseRegionSettings(gSavedSettings.getBOOL("UseEnvironmentFromRegion"), gSavedSettings.getBOOL("PhoenixInterpolateSky"));
}

void wlfPanel_AdvSettings::onChangeWWPresetName(LLUICtrl* ctrl, void * userData)
{
	LLComboBox * combo_box = static_cast<LLComboBox*>(ctrl);
	
	if(combo_box->getSimple() == "")
	{
		return;
	}

	const std::string& wwset = combo_box->getSelectedValue().asString();
	if (LLWaterParamManager::getInstance()->hasParamSet(wwset))
	{
		LLEnvManagerNew::instance().setUseWaterPreset(wwset, gSavedSettings.getBOOL("PhoenixInterpolateWater"));
	}
	else
	{
		//if that failed, use region's
		// LLEnvManagerNew::instance().useRegionWater();
		LLEnvManagerNew::instance().setUseWaterPreset("Default", gSavedSettings.getBOOL("PhoenixInterpolateWater"));
	}
}

void wlfPanel_AdvSettings::onChangeWLPresetName(LLUICtrl* ctrl, void * userData)
{
	LLComboBox * combo_box = static_cast<LLComboBox*>(ctrl);
	
	if(combo_box->getSimple() == "")
	{
		return;
	}

	const LLWLParamKey key(combo_box->getSelectedValue().asString(), LLEnvKey::SCOPE_LOCAL);
	if (LLWLParamManager::getInstance()->hasParamSet(key))
	{
		LLEnvManagerNew::instance().setUseSkyPreset(key.name, gSavedSettings.getBOOL("PhoenixInterpolateSky"));
	}
	else
	{
		//if that failed, use region's
		// LLEnvManagerNew::instance().useRegionSky();
		LLEnvManagerNew::instance().setUseSkyPreset("Default", gSavedSettings.getBOOL("PhoenixInterpolateSky"));
	}
}

void wlfPanel_AdvSettings::onClickWWNext(void* user_data)
{
	wlfPanel_AdvSettings* self = (wlfPanel_AdvSettings*) user_data;

	S32 index = self->mWaterPresetCombo->getCurrentIndex();
	index++;
	if (index == self->mWaterPresetCombo->getItemCount())
		index = 0;
	self->mWaterPresetCombo->setCurrentByIndex(index);

	wlfPanel_AdvSettings::onChangeWWPresetName(self->mWaterPresetCombo, self);
}

void wlfPanel_AdvSettings::onClickWWPrev(void* user_data)
{
	wlfPanel_AdvSettings* self = (wlfPanel_AdvSettings*) user_data;

	S32 index = self->mWaterPresetCombo->getCurrentIndex();
	if (index == 0)
		index = self->mWaterPresetCombo->getItemCount();
	index--;
	self->mWaterPresetCombo->setCurrentByIndex(index);

	wlfPanel_AdvSettings::onChangeWWPresetName(self->mWaterPresetCombo, self);
}

void wlfPanel_AdvSettings::onClickWLNext(void* user_data)
{
	wlfPanel_AdvSettings* self = (wlfPanel_AdvSettings*) user_data;

	S32 index = self->mSkyPresetCombo->getCurrentIndex();
	index++;
	if (index == self->mSkyPresetCombo->getItemCount())
		index = 0;
	self->mSkyPresetCombo->setCurrentByIndex(index);

	wlfPanel_AdvSettings::onChangeWLPresetName(self->mSkyPresetCombo, self);
}

void wlfPanel_AdvSettings::onClickWLPrev(void* user_data)
{
	wlfPanel_AdvSettings* self = (wlfPanel_AdvSettings*) user_data;

	S32 index = self->mSkyPresetCombo->getCurrentIndex();
	if (index == 0)
		index = self->mSkyPresetCombo->getItemCount();
	index--;
	self->mSkyPresetCombo->setCurrentByIndex(index);

	wlfPanel_AdvSettings::onChangeWLPresetName(self->mSkyPresetCombo, self);
}

void wlfPanel_AdvSettings::onOpenAdvancedSky(void* userData)
{
	LLFloaterWindLight::show();
}

void wlfPanel_AdvSettings::onOpenAdvancedWater(void* userData)
{
	LLFloaterWater::show();
}

void wlfPanel_AdvSettings::onChangeDayTime(LLUICtrl* ctrl, void* userData)
{
	LLSliderCtrl* sldr = static_cast<LLSliderCtrl*>(ctrl);

	if (sldr) {
		// deactivate animator
		LLWLParamManager::getInstance()->mAnimator.deactivate();

		F32 val = sldr->getValueF32() + 0.25f;
		if(val > 1.0) 
		{
			val--;
		}

		LLWLParamManager::getInstance()->mAnimator.setDayTime((F64)val);
		LLWLParamManager::getInstance()->mAnimator.update(
			LLWLParamManager::getInstance()->mCurParams);
	}
}

void wlfPanel_AdvSettings::populateWaterPresetsList()
{
	mWaterPresetCombo->removeall();

	std::list<std::string> presets;
	LLWaterParamManager::getInstance()->getPresetNames(presets);

	for (std::list<std::string>::const_iterator it = presets.begin(); it != presets.end(); ++it)
	{
		mWaterPresetCombo->add(*it);
	}

	//populate the combos with "Default" if using region settings
	if (gSavedSettings.getBOOL("UseEnvironmentFromRegion"))
	{
		mWaterPresetCombo->selectByValue("Default");
	}
	else
	{
		// Select the current presets in combo boxes.
		mWaterPresetCombo->selectByValue(LLEnvManagerNew::instance().getWaterPresetName());
	}
}

void wlfPanel_AdvSettings::populateSkyPresetsList()
{
	mSkyPresetCombo->removeall();

	LLWLParamManager::preset_name_list_t local_presets;
	LLWLParamManager::getInstance()->getLocalPresetNames(local_presets);

	for (LLWLParamManager::preset_name_list_t::const_iterator it = local_presets.begin(); it != local_presets.end(); ++it)
	{
		mSkyPresetCombo->add(*it);
	}

	//populate the combos with "Default" if using region settings
	if (gSavedSettings.getBOOL("UseEnvironmentFromRegion"))
	{
		mSkyPresetCombo->selectByValue("Default");
	}
	else
	{
		// Select the current presets in combo boxes.
		mSkyPresetCombo->selectByValue(LLEnvManagerNew::instance().getSkyPresetName());
	}
}

// void wlfPanel_AdvSettings::populateDayCyclePresetsList()
// {
	// mDayCyclePresetCombo->removeall();

	// LLDayCycleManager::preset_name_list_t user_days, sys_days;
	// LLDayCycleManager::instance().getPresetNames(user_days, sys_days);

	// // Add user days.
	// for (LLDayCycleManager::preset_name_list_t::const_iterator it = user_days.begin(); it != user_days.end(); ++it)
	// {
		// mDayCyclePresetCombo->add(*it);
	// }

	// if (user_days.size() > 0)
	// {
		// mDayCyclePresetCombo->addSeparator();
	// }

	// // Add system days.
	// for (LLDayCycleManager::preset_name_list_t::const_iterator it = sys_days.begin(); it != sys_days.end(); ++it)
	// {
		// mDayCyclePresetCombo->add(*it);
	// }

	//populate the combos with "Default" if using region settings
	// if (gSavedSettings.getBOOL("UseEnvironmentFromRegion"))
	// {
		// mDayCyclePresetCombo->selectByValue("Default");
	// }
	// else
	// {
		// Select the current presets in combo boxes.
		// mDayCyclePresetCombo->selectByValue(LLEnvManagerNew::instance().getDayCycleName());
	// }
// }

void wlfPanel_AdvSettings::updateTimeSlider()
{
	
	F32 val = LLWLParamManager::getInstance()->mAnimator.getDayTime() - 0.25f;
	if(val < 0.0)
	{
		val++;
	}
	mTimeSlider->setValue(val);
}
