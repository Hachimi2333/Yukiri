#pragma once

#include <optional>
#include <string>

namespace Yukiri
{
class Widget;
struct AccessibilityInfo
{
	enum class Role
	{
		Automatic, Group, Text, Image, Button, CheckBox, RadioButton, Switch, Slider, ProgressBar,
		Edit, ComboBox, Tab, TabItem, ScrollView, List, ListItem, Tree, TreeItem, Table, DataItem,
		Header, HeaderItem, MenuBar, Menu, MenuItem, ToolBar, ToolTip, StatusBar, Separator, Dialog, Window, TitleBar, NumberBox
	};
	std::string m_Name;
	std::string m_Description;
	std::string m_AutomationId;
	Role m_Role = Role::Automatic;
	Widget* m_Label = nullptr;
	std::optional<bool> m_IsContentElement;
	bool m_Hidden = false;
};
}
