-- NOTE:
--   This script was taken from: https://wiki.wxwidgets.org/Premake4
--   And modified by Miss to fix some issues and work with Premake5
--
-- Use like:
--   flags "WinMain"
--   configuration "Debug"
--     wx_config { Root=WX_DIR, Debug="yes", Static="yes", Unicode="yes", Version="3.1", Libs="core,adv,gl,aui,propgrid" }
--   configuration "Release"
--     wx_config { Root=WX_DIR, Debug="np", Static="yes", Unicode="yes", Version="3.1", Libs="core,adv,gl,aui,propgrid" }

-- use_wxwidgets.lua
-- wxWidgets configuration file for premake4
--
-- authors: laurent.humbertclaude@gmail.com
--          v.krishnakumar@gmail.com

-- optional root folder of wxwidgets installation folder
--

newoption  {
	trigger     =   "wx_root",
	value       =   "PATH",
	description =   "Path to wxwidgets root folder, by default, WXWIN envvar will be used or wx-config found in path on POSIX"
}

function shellout(str)
	if USE_RUNTIME then
		return { "`" .. str .. "`" }
	end

	local out_links = {}
	local out_includes = {}

	ret = {}
	cmd_result = os.outputof(str)
	for i in (cmd_result:match'^%s*(.*%S)' or ''):gmatch('%S+') do
		if string.sub(i, 1, 2) == "-l" then
			table.insert(out_links, string.sub(i, 3))
		elseif string.sub(i, 1, 2) == "-I" then
			table.insert(out_includes, string.sub(i, 3))
		else
			table.insert(ret, i)
		end
	end

	links(out_links)
	includedirs(out_includes)

	return ret
end

-- The wx_config the parameters are.
--          Root    : path to wx root folder. Can be left empty if WXWIN is defined
--                      or if wx-config is accessible.
--          Debug   : "yes" use debug version of wxwidgets. Default to "no"
--          Version : one of '2.4', '2.5', '2.6', '2.7', '2.8', '2.9'. Default to '2.9'
--          Static  : indicates how wx is to be linked. Values are
--                      either "yes" for static linking or "no" for shared linking, Default to "no"
--          Unicode : use "yes" for unicode or "no" for ansi version.
--                      ansi version only available up to 2.8
--                      Default to "yes"
--          Universal : use universal configuration. Default to "no"
--          Libs    : a list of wx libraries that you want to link with.
--                      eg: "aui,media,html"
--                      Default to "richtext,aui,xrc,qa,html,adv,core,xml,net"; base is implicit
--          WindowsCompiler : compiler used to compile windows libraries ( "vc" or "gcc" )

function wx_config(options)

	local wrongParam = false
	local allowedWxOptions = {"Root", "Debug", "Host", "Version", "Static", "Unicode", "Universal", "Libs", "WindowsCompiler" }
	for option in pairs(options) do
		if not table.contains(allowedWxOptions, option) then
			print ("unrecognized option '"..option.. "'")
			wrongParam = true
		end
	end
	if wrongParam then print("valid options are : '" .. table.concat(allowedWxOptions, "', '").."'") end

	wx_config_Private( options.Root or "",
				options.Debug or "",
				options.Host or "",
				options.Version or "2.9",
				options.Static or "",
				options.Unicode or "yes",
				options.Universal or "",
				options.Libs or "richtext,aui,xrc,qa,html,adv,core,xml,net", -- base is implicit, std not valid
				options.WindowsCompiler or "vc"
				)
end

function wx_config_Private(wxRoot, wxDebug, wxHost, wxVersion, wxStatic, wxUnicode, wxUniversal, wxLibs, wxWindowsCompiler)
	-- some options are not allowed for newer version of wxWidgets
	if wxVersion > "2.8" then -- alphabetical comparison may fail...
	--    wxDebug = "" -- 2.9 still make debug libraries
		wxUnicode = "yes"
	end

	--wx_root=PATH override wxRoot parameter
	if _OPTIONS and _OPTIONS["wx_root"] then
		print ("seen option '--wx_root=" .. _OPTIONS["wx_root"] .. "' overriding default root = '" .. wxRoot .. "'")
		wxRoot = _OPTIONS["wx_root"]
	end
	-- the environment variable WXWIN override both wxRoot parameter and --wx_root option
	if os.getenv('WXWIN') then wxRoot = os.getenv('WXWIN') end

	if wxUnicode == "yes" then
		defines { "_UNICODE" }
		characterset "Unicode"
	end

	if wxDebug == "yes" then defines { "__WXDEBUG__" }
	elseif wxDebug == "no" then optimize("On") end

	if wxStatic == "yes" then
		--flags { "StaticRuntime" }
		if os.target() == "windows" then
			links { "ComCtl32", "RpcRT4", "WinMM", "AdvAPI32", "WSock32" }
		end
	else
		defines { "WXUSINGDLL" }
	end


	-- function to compensate lack of wx-config program on windows
	-- but wait, look at http://sites.google.com/site/wxconfig/ for one !
	function wx_config_for_windows(wxWindowsCompiler)
		local wxBuildType = ""  -- buildtype is one of "", "u", "d" or "ud"
		local wxDebugSuffix = "" -- debug buildsuffix is for support libraries only
		if wxUnicode ~= "no" then wxBuildType = wxBuildType .. "u" end
		if wxDebug == "yes" then
			wxBuildType = wxBuildType .. "d"
			wxDebugSuffix = "d"
		end

		local wxLibPath = path.join(wxRoot, "lib")
		wxLibPath = path.join(wxLibPath, wxWindowsCompiler .. "_" .. iif(wxStatic == 'yes', 'lib', 'dll'))
		-- common defines
		defines{ "__WXMSW__" }

		-- common include path
		includedirs {
			path.join(wxRoot, "include"),
			path.join(wxLibPath, "msw" .. wxBuildType)   -- something like "%WXWIN%\lib\vc_lib\mswud" to find "wx/setup.h"
			}

		-- common library path
		libdirs { wxLibPath }

		-- add the libs
		libVersion = string.gsub(wxVersion, '%.', '') -- remove dot from version
		links { "wxbase"..libVersion..wxBuildType } -- base lib
		for i, lib in ipairs(string.explode(wxLibs, ",")) do
			local libPrefix = 'wxmsw'
			if lib == "xml" or lib == "net" or lib == "odbc" then
				libPrefix = 'wxbase'
			end
			links { libPrefix..libVersion..wxBuildType..'_'..lib}
		end
		-- link with support libraries
		for i, lib in ipairs({"wxjpeg", "wxpng", "wxzlib", "wxtiff", "wxexpat"}) do
			links { lib..wxDebugSuffix }
		end
		links { "wxregex" .. wxBuildType }
	end

	-- use wx-config to figure out build parameters
	function wx_config_for_posix()
		local configCmd = "wx-config"  -- this is the wx-config command line
		if wxRoot ~= "" and os.target() ~= 'macosx' then configCmd = path.join(wxRoot, "bin/wx-config") end

		local function checkYesNo(value, option)
			if value == "" then return "" end
			if value == "yes" or value == "no" then return " --"..option.."="..value end
			error("wx"..option..' can only be "yes", "no" or empty' )
		end

		configCmd = configCmd .. checkYesNo(wxDebug, "debug")
		configCmd = configCmd .. checkYesNo(wxStatic, "static")
		configCmd = configCmd .. checkYesNo(wxUnicode, "unicode")
		configCmd = configCmd .. checkYesNo(wxUniversal, "universal")
		if wxHost ~= "" then configCmd = configCmd .. " --host=" .. wxHost end
		if wxVersion ~= "" then configCmd = configCmd .. " --version=" .. wxVersion end

		buildFlags = shellout(configCmd .." --cxxflags")
		libFlags = shellout(configCmd .." --libs " .. wxLibs)

		if os.target() == 'macosx' then
			-- on osx we need to concatenate the "-framework" and "Cocoa" parts, since they are split and it confuses GENie
			for i, v in ipairs(libFlags) do
				if v == '-framework' then
					libFlags[i] = '-framework ' .. libFlags[i + 1]
					libFlags[i + 1] = ''
				end
			end
		end

		-- set the parameters to the curent configuration
		buildoptions(buildFlags)
		linkoptions(libFlags)
	end

-- BUG: here, using any configuration() function will reset the current filter
--      and apply configuration to all project configuration...
--      see http://industriousone.com/post/add-way-refine-configuration-filter
--      and http://sourceforge.net/tracker/?func=detail&aid=2936443&group_id=71616&atid=531881
--~     configuration "not windows"
--~         wx_config_for_posix()
--~     configuration "vs*"
--~         wx_config_for_windows("vc")
--~     configuration {"windows", "codeblocks or gmake or codelitle"}
--~         wx_config_for_windows("gcc")
	if os.target() ~= "windows" then
		wx_config_for_posix()
	else
		local allowedCompiler = {"vc", "gcc"}
		if not table.contains( allowedCompiler, wxWindowsCompiler) then
			print( "wrong wxWidgets Compiler specified('"..wxWindowsCompiler.."'), should be one of '".. table.concat(allowedCompiler, "', '").."'" )
			wxWindowsCompiler = "vc"
		end
--~  BUG/WISH: I need a function like compiler.get() that return the project/configuration compiler
--~         local wxWindowsCompiler = "vc"
--~  BUG? --cc=compiler standard premake option is not registered in the _OPTIONS array
--~         if _OPTIONS and _OPTIONS["cc"] then
--~             wxWindowsCompiler = _OPTIONS.cc
--~             print("seen option '--cc=" .. _OPTIONS["cc"] .. "' overriding default cc='vc'")
--~         end
		wx_config_for_windows(wxWindowsCompiler)
	end
end
