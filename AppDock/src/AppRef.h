#pragma once

#include <string>
#include "Utils/json.hpp"

using json = nlohmann::json;

// Reference and settings to applications that can be 
// opened with 
class AppRef
{
public:
	/// <summary>
	/// A runtime version to access the default value.
	/// </summary>
	static const AppRef _default;

public:
	std::wstring label				= L"";
	std::wstring command			= L"";
	std::wstring startDir			= L"";
	int msBeforeAttach				= 500;
	bool closeIfFail				= false;
	bool startShown					= true;

public:
	AppRef();
	AppRef(const json& entry);
	bool Load(const json& entry);
	bool IsValid() const;
	json ToJSON() const;
};