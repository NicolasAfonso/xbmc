/*
 *      Copyright (C) 2011-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "addons/Skin.h"
#include "interfaces/legacy/ModuleXbmc.h"
#include "GUIOperations.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "GUIInfoManager.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/Key.h"
#include "interfaces/Builtins.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "addons/AddonManager.h"
#include "settings/Settings.h"
#include "utils/Variant.h"
#include "guilib/StereoscopicsManager.h"
#include "windowing/WindowingFactory.h"
#include "guilib/GUIWindowManager.h"
#include "InputOperations.h"


using namespace std;
using namespace JSONRPC;
using namespace ADDON;

JSONRPC_STATUS CGUIOperations::GetProperties(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVariant properties = CVariant(CVariant::VariantTypeObject);
  for (unsigned int index = 0; index < parameterObject["properties"].size(); index++)
  {
    CStdString propertyName = parameterObject["properties"][index].asString();
    CVariant property;
    JSONRPC_STATUS ret;
    if ((ret = GetPropertyValue(propertyName, property)) != OK)
      return ret;

    properties[propertyName] = property;
  }

  result = properties;

  return OK;
}

JSONRPC_STATUS CGUIOperations::ActivateWindow(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVariant params = parameterObject["parameters"];
  std::string cmd = "ActivateWindow(" + parameterObject["window"].asString();
  for (CVariant::iterator_array param = params.begin_array(); param != params.end_array(); param++)
  {
    if (param->isString() && !param->empty())
      cmd += "," + param->asString();
  }
  cmd += ")";
  CBuiltins::Execute(cmd);

  return ACK;
}

JSONRPC_STATUS CGUIOperations::ShowNotification(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  string image = parameterObject["image"].asString();
  string title = parameterObject["title"].asString();
  string message = parameterObject["message"].asString();
  unsigned int displaytime = (unsigned int)parameterObject["displaytime"].asUnsignedInteger();

  if (image.compare("info") == 0)
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, title, message, displaytime);
  else if (image.compare("warning") == 0)
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, title, message, displaytime);
  else if (image.compare("error") == 0)
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, title, message, displaytime);
  else
    CGUIDialogKaiToast::QueueNotification(image, title, message, displaytime);

  return ACK;
}

JSONRPC_STATUS CGUIOperations::SetFullscreen(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if ((parameterObject["fullscreen"].isString() &&
       parameterObject["fullscreen"].asString().compare("toggle") == 0) ||
      (parameterObject["fullscreen"].isBoolean() &&
       parameterObject["fullscreen"].asBoolean() != g_application.IsFullScreen()))
    CApplicationMessenger::Get().SendAction(CAction(ACTION_SHOW_GUI));
  else if (!parameterObject["fullscreen"].isBoolean() && !parameterObject["fullscreen"].isString())
    return InvalidParams;

  return GetPropertyValue("fullscreen", result);
}

JSONRPC_STATUS CGUIOperations::SetStereoscopicMode(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CAction action = CStereoscopicsManager::Get().ConvertActionCommandToAction("SetStereoMode", parameterObject["mode"].asString());
  if (action.GetID() != ACTION_NONE)
  {
    CApplicationMessenger::Get().SendAction(action);
    return ACK;
  }

  return InvalidParams;
}

JSONRPC_STATUS CGUIOperations::GetStereoscopicModes(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  for (int i = RENDER_STEREO_MODE_OFF; i < RENDER_STEREO_MODE_COUNT; i++)
  {
    RENDER_STEREO_MODE mode = (RENDER_STEREO_MODE) i;
    if (g_Windowing.SupportsStereo(mode))
      result["stereoscopicmodes"].push_back(GetStereoModeObjectFromGuiMode(mode));
  }

  return OK;
}

JSONRPC_STATUS CGUIOperations::GetCurrentListDisplayed(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result){

	CGUIWindow * window = g_windowManager.GetWindow(g_windowManager.GetFocusedWindow());

	if(window->HasListItems()){
		CFileItemPtr listItem = window->GetCurrentListItem(0);

		CVariant tmpItem;
		tmpItem["itemId"].push_back(listItem->GetLabel());
		result.push_back(tmpItem);


		CFileItemPtr listItemTmp = window->GetCurrentListItem(1);
		int id = 1;
		while(!listItemTmp->GetLabel().Equals(listItem->GetLabel())) {
			tmpItem.clear();
			tmpItem["itemId"].push_back(CVariant(listItemTmp->GetLabel()));
			result.push_back(tmpItem);

			id++;
			listItemTmp = window->GetCurrentListItem(id);
		}
	}
	else {
		result.push_back(CVariant());
	}

	return OK;
}

JSONRPC_STATUS CGUIOperations::GetCurrentMainMenu(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result){
	
	CVariant menuContent;
	std::vector<string> categoriesLabels;
	std::vector<string> categoriesLabels2;
	std::string cond;

	categoriesLabels.clear();
	categoriesLabels.push_back("Music");
	categoriesLabels.push_back("Videos");
	categoriesLabels.push_back("Movie");
	categoriesLabels.push_back("TVShow");
	categoriesLabels.push_back("Pictures");
	categoriesLabels.push_back("Programs");
	categoriesLabels.push_back("Weather");

	// this is pretty uggly but the Confluence skin and XBMC API do not use the same syntax
	categoriesLabels2.clear();
	categoriesLabels2.push_back("Music");
	categoriesLabels2.push_back("Video");
	categoriesLabels2.push_back("Movies");
	categoriesLabels2.push_back("TVShows");

	for(std::size_t i=0;i<categoriesLabels.size();i++) {

		 // we first take in consideration the user preferences setted in the Confluence Skin settings
		 // if the user do not use this skin, we always send the main menu item, unless the library corresponding to this item is empty
		 cond.clear();
		 cond = "Skin.HasSetting(HomeMenuNo" + categoriesLabels[i] + "Button)";

		 if (!(XBMCAddon::xbmc::getCondVisibility( cond.c_str() ) )) {

			 if (categoriesLabels[i] == "Weather") {
				 cond.clear();
				 cond = "IsEmpty(Weather.Plugin)";
				 if (XBMCAddon::xbmc::getCondVisibility( cond.c_str() ) )
					 break;
			 }

			 // We use the Library.Hascontent method. Valid parameters are only Video, Music, Movies, TVShows, MusicVideos & MovieSets
			 if( categoriesLabels[i] != "Programs" && categoriesLabels[i] != "Weather" && categoriesLabels[i] != "Pictures"){
					cond.clear();
					cond = "Library.HasContent("+ categoriesLabels2[i] +")";
				 
					if (XBMCAddon::xbmc::getCondVisibility( cond.c_str() ) ) {
						menuContent.clear();
						menuContent["menuId"].push_back(categoriesLabels[i]);
						result.push_back(menuContent);
				 }
				}

			 else {
				menuContent.clear();
				menuContent["menuId"].push_back(categoriesLabels[i]);
				result.push_back(menuContent);
			}
	 	 }
	}

	 if (XBMCAddon::xbmc::getCondVisibility("System.GetBool(pvrmanager.enabled)")) {
		menuContent.clear();
		menuContent["menuId"].push_back("PVR");
		result.push_back(menuContent);
	 }

	 if (XBMCAddon::xbmc::getCondVisibility("System.HasMediaDVD")) {
		 menuContent.clear();
		 menuContent["menuId"].push_back("Disk");
		 result.push_back(menuContent);
	 }
	
	 if (result.empty()) {
		menuContent.clear();
		result.push_back(menuContent);
	 }

	return OK;
}

JSONRPC_STATUS CGUIOperations::NavigateInListItem(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result){

	CGUIWindow *window = g_windowManager.GetWindow(g_windowManager.GetFocusedWindow());

	if(window->HasListItems()){
		CStdString param = parameterObject["SelectedItem"].asString();

		CFileItemPtr firstListItem, listItemTmp;
		CFileItemPtr selectedItemToTrigger;
		bool ItemFound = false;
		int id =0;
		int deltaPosition;

		firstListItem = window->GetCurrentListItem(0);
		if(firstListItem->GetLabel().Equals(param)) {
			selectedItemToTrigger = firstListItem;
			deltaPosition = id;
			ItemFound = true;
		}
		else {
			id++;
			listItemTmp = window->GetCurrentListItem(id);
			while( !(firstListItem->GetLabel().Equals(listItemTmp->GetLabel().c_str())) ) {
				if(listItemTmp->GetLabel().Equals(param)) {
					selectedItemToTrigger = listItemTmp;
					deltaPosition = id;
					ItemFound = true;
					break;
				}
				id++;
				listItemTmp = window->GetCurrentListItem(id);
			}
		}

		if(ItemFound) {
			for(int i=0;i<deltaPosition;i++) {
				CInputOperations::Down(method, transport, client, parameterObject, result);
			}
			CInputOperations::Select(method, transport, client, parameterObject, result);
			result["res"].push_back(CVariant("OK - selectedItem :"+selectedItemToTrigger->GetLabel()));
		}
		else {
			return InvalidParams;
		}
	}
	else {
			return InvalidParams;
	}
	return OK;
}

JSONRPC_STATUS CGUIOperations::GetPropertyValue(const CStdString &property, CVariant &result)
{
  if (property.Equals("currentwindow"))
  {
    result["label"] = g_infoManager.GetLabel(g_infoManager.TranslateString("System.CurrentWindow"));
    result["id"] = g_windowManager.GetFocusedWindow();
  }
  else if (property.Equals("currentcontrol"))
    result["label"] = g_infoManager.GetLabel(g_infoManager.TranslateString("System.CurrentControl"));
  else if (property.Equals("skin"))
  {
    CStdString skinId = CSettings::Get().GetString("lookandfeel.skin");
    AddonPtr addon;
    CAddonMgr::Get().GetAddon(skinId, addon, ADDON_SKIN);

    result["id"] = skinId;
    if (addon.get())
      result["name"] = addon->Name();
  }
  else if (property.Equals("fullscreen"))
    result = g_application.IsFullScreen();
  else if (property.Equals("stereoscopicmode"))
    result = GetStereoModeObjectFromGuiMode( CStereoscopicsManager::Get().GetStereoMode() );
  else
    return InvalidParams;

  return OK;
}

CVariant CGUIOperations::GetStereoModeObjectFromGuiMode(const RENDER_STEREO_MODE &mode)
{
  CVariant modeObj(CVariant::VariantTypeObject);
  modeObj["mode"] = CStereoscopicsManager::Get().ConvertGuiStereoModeToString(mode);
  modeObj["label"] = CStereoscopicsManager::Get().GetLabelForStereoMode(mode);
  return modeObj;
}
