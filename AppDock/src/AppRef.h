#pragma once

#include <string>
#include "Utils/json.hpp"

using json = nlohmann::json;

/// <summary>
/// Reference and settings to applications that can be opened 
/// from the launch list.
/// </summary>
class AppRef
{
public:
	/// <summary>
	/// A runtime version to access the default value. This allows use to set
	/// unspecified parameters to AppRef's default C++ defined values.
	/// </summary>
	static const AppRef _default;

public:
	/// <summary>
	/// The text to show in the menu, to represent this entry.
	/// </summary>
	std::wstring label				= L"";

	/// <summary>
	/// The command to run to create the process. It's assumed the 
	/// process will have a Window that can be found by directly
	/// querying it with Windows API functions.
	/// 
	/// The command may include command line arguments.
	/// </summary>
	std::wstring command			= L"";

	/// <summary>
	/// The working directory to launch the application in.
	/// </summary>
	std::wstring startDir			= L"";

	/// <summary>
	/// When launching the application, the number of milliseconds
	/// to wait for attaching. This will be a blocking function.
	/// </summary>
	int msBeforeAttach				= 500;

	/// <summary>
	/// If the application fails to find a window and dock it, should
	/// the application process be aborted?
	/// </summary>
	bool closeIfFail				= false;

	/// <summary>
	/// Before docking the window, should be try to make the application
	/// visible? If not, we try to make it invisible, and only have it
	/// visible after docking it.
	/// 
	/// If not started shown, it's a more seamless process of creating a
	/// docked window, but can cause issues if an error happens during
	/// docking - leaving the user without a way to close the window
	/// without finding it in Task Manager.
	/// 
	/// Note that depending on how the target application works (that we
	/// are starting), startShown may fail to be respected.
	/// </summary>
	bool startShown					= true;

public:
	AppRef();

	/// <summary>
	/// Constructor.
	/// 
	/// Loads values from json.
	/// </summary>
	/// <param name="entry"></param>
	AppRef(const json& entry);

	/// <summary>
	/// 
	/// </summary>
	/// <param name="entry"></param>
	/// <returns></returns>
	bool Load(const json& entry);

	/// <summary>
	/// Check if the application reference is valid.
	/// 
	/// Note that this only checks if the object is valid. Not to be
	/// confused with the processed that would be spawned is/would-be
	/// valid.
	/// </summary>
	/// <returns>
	/// True if the required elements of a valid AppRef are set in the object.
	/// </returns>
	bool IsValid() const;

	/// <summary>
	/// Converts the AppRef object to a JSON representation.
	/// 
	/// Note that this and Load() need to agree with each other.
	/// </summary>
	/// <returns>The JSON representation.</returns>
	json ToJSON() const;
};