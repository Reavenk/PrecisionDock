#include "AppRef.h"

const AppRef AppRef::_default;

AppRef::AppRef()
{}

AppRef::AppRef(const json& entry)
{
	this->Load(entry);
}

// Utility used in AppRef::Load
std::wstring ToWString(const std::string& str)
{
	return std::wstring(str.begin(), str.end());
}

bool AppRef::Load(const json& entry)
{
	if(	
		entry.contains("label") == false || 
		entry["label"].is_string() == false)
	{
		return false;
	}

	if(	
		entry.contains("command") == false ||
		entry["command"].is_string() == false)
	{ 
		return false;
	}

	this->label		= ToWString(entry["label"	]);
	this->command	= ToWString(entry["command"	]);

	if(
		entry.contains("startat") == true && 
		entry["startat"].is_string() == true)
	{ 
		this->startDir = ToWString(entry["startat"]);
	}

	if(
		entry.contains("warmup") == true && 
		entry["warmup"].is_number_integer() == true)
	{
		this->msBeforeAttach = entry["warmup"];
	}

	if(
		entry.contains("closefails") == true && 
		entry["closefails"].is_boolean() == true)
	{ 
		this->closeIfFail = entry["closefails"];
	}

	if (
		entry.contains("startshown") == true &&
		entry["startshown"].is_boolean() == true)
	{
		this->startShown = entry["startshown"];
	}

	return true;
}

bool AppRef::IsValid() const
{
	return 
		this->label.empty() == false && 
		this->command.empty() == false;
}

json AppRef::ToJSON() const
{
	json ret = json::object();

	ret["label"]		= this->label;
	ret["command"]		= this->command;

	if(this->startDir		!= _default.startDir)
		ret["startat"]		= this->startDir;

	if(this->msBeforeAttach != _default.msBeforeAttach)
		ret["warmup"]		= this->msBeforeAttach;

	if(this->closeIfFail	!= _default.closeIfFail)
		ret["closefails"]	= this->closeIfFail;

	if(this->startShown		!= _default.startShown)
		ret["startshown"]	= this->startShown;

	return ret;
}