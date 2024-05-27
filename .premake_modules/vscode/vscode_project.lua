--
-- Name:        vscode_project.lua
-- Purpose:     Generate a vscode C/C++ project file.
-- Author:      Ryan Pusztai
-- Modified by: Andrea Zanellato
--              Manu Evans
--              Tom van Dijck
--              Yehonatan Ballas
--              Jan "GamesTrap" Schürkamp
-- Created:     2013/05/06
-- Updated:     2022/12/29
-- Copyright:   (c) 2008-2020 Yehonatan Ballas, Jason Perkins and the Premake project
--              (c) 2022-2023 Jan "GamesTrap" Schürkamp
--

local p = premake
local tree = p.tree
local project = p.project
local config = p.config
local vscode = p.modules.vscode

vscode.project = {}
local m = vscode.project

function m.getcompiler(cfg)
	local toolset = p.tools[_OPTIONS.cc or cfg.toolset or p.CLANG]
	if not toolset then
		error("Invalid toolset '" + (_OPTIONS.cc or cfg.toolset) + "'")
	end
	return toolset
end

function m.getcorecount()
	local cores = 0

	if os.host() == "windows" then
		local result, errorcode = os.outputof("wmic cpu get NumberOfCores")
		for core in result:gmatch("%d+") do
			cores = cores + core
		end
	elseif os.host() == "linux" then
		local result, errorcode = os.outputof("nproc")
		cores = result
	end

	cores = math.ceil(cores * 0.75)

	if cores <= 0 then
		cores = 1
	end

	return cores
end

function m.vscode_tasks(prj, tasksFile)
	local output = ""

	local slnLocation = ''
    if prj.vscode_solution ~= nil then
        slnLocation = '/'..prj.vscode_solution
    end

	for cfg in project.eachconfig(prj) do
		local buildName = "Build " .. prj.name .. " (" .. cfg.name .. ")"
		local target = path.getrelative(prj.workspace.location, prj.location)
		target = target:gsub("/", "\\\\")

		output = output .. '\t\t{\n'
		output = output .. string.format('\t\t\t"label": "%s",\n', buildName)
		output = output .. '\t\t\t"type": "shell",\n'
		output = output .. '\t\t\t"linux":\n'
		output = output .. '\t\t\t{\n'
		output = output .. string.format('\t\t\t\t"command": "clear && time make config=%s %s -j%s",\n', string.lower(cfg.name):gsub("%|", "_"), prj.name, m.getcorecount())
		output = output .. '\t\t\t\t"problemMatcher": "$gcc",\n'
		output = output .. '\t\t\t},\n'
		output = output .. '\t\t\t"windows":\n'
		output = output .. '\t\t\t{\n'
		output = output .. '\t\t\t\t"command": "cls && msbuild",\n'
		output = output .. '\t\t\t\t"args":\n'
		output = output .. '\t\t\t\t[\n'
		output = output .. string.format('\t\t\t\t\t"/m:%s",\n', m.getcorecount())
		output = output .. string.format('\t\t\t\t\t"${workspaceRoot}%s/%s.sln",\n', slnLocation, prj.workspace.name)
		output = output .. string.format('\t\t\t\t\t"/p:Configuration=%s",\n', cfg.name)
		output = output .. string.format('\t\t\t\t\t"/t:%s",\n', target)
		output = output .. '\t\t\t\t],\n'
		output = output .. '\t\t\t\t"problemMatcher": "$msCompile",\n'
		output = output .. '\t\t\t},\n'
		output = output .. '\t\t\t"group":\n'
		output = output .. '\t\t\t{\n'
		output = output .. '\t\t\t\t"kind": "build",\n'
		output = output .. '\t\t\t},\n'
		output = output .. '\t\t},\n'
	end

	tasksFile:write(output)
end

function m.vscode_launch(prj, launchFile)
	local output = ""

	for cfg in project.eachconfig(prj) do
		if cfg.kind == "ConsoleApp" or cfg.kind == "WindowedApp" then --Ignore non executable configuration(s) in launch.json
			local buildName = "Build " .. prj.name .. " (" .. cfg.name .. ")"
			local target = path.getrelative(prj.workspace.location, prj.location)
			local gdbPath = ""
			if os.host() == "linux" then
				gdbPath = os.outputof("which gdb")
			end
			local programPath = path.getrelative(prj.workspace.location, cfg.buildtarget.abspath)

			output = output .. '\t\t{\n'
			output = output .. string.format('\t\t\t"name": "Run %s (%s)",\n', prj.name, cfg.name)
			output = output .. '\t\t\t"request": "launch",\n'
			output = output .. '\t\t\t"type": "cppdbg",\n'
			output = output .. string.format('\t\t\t"program": "${workspaceRoot}/%s",\n', programPath)
			output = output .. '\t\t\t"linux":\n'
			output = output .. '\t\t\t{\n'
			output = output .. string.format('\t\t\t\t"name": "Run %s (%s)",\n', prj.name, cfg.name)
			output = output .. '\t\t\t\t"type": "cppdbg",\n'
			output = output .. '\t\t\t\t"request": "launch",\n'
			output = output .. string.format('\t\t\t\t"program": "${workspaceRoot}/%s",\n', programPath)
			output = output .. '\t\t\t\t"externalConsole": false,\n'
			output = output .. string.format('\t\t\t\t"miDebuggerPath": "%s",\n', gdbPath)
			output = output .. '\t\t\t\t"MIMode": "gdb",\n'
			output = output .. '\t\t\t\t"setupCommands":\n'
			output = output .. '\t\t\t\t[\n'
			output = output .. '\t\t\t\t\t{\n'
			output = output .. '\t\t\t\t\t\t"text": "-enable-pretty-printing",\n'
			output = output .. '\t\t\t\t\t\t"description": "enable pretty printing",\n'
			output = output .. '\t\t\t\t\t\t"ignoreFailures": true,\n'
			output = output .. '\t\t\t\t\t},\n'
			output = output .. '\t\t\t\t],\n'
			output = output .. '\t\t\t},\n'
			output = output .. '\t\t\t"windows":\n'
			output = output .. '\t\t\t{\n'
			output = output .. string.format('\t\t\t\t"name": "Run %s (%s)",\n', prj.name, cfg.name)
			output = output .. '\t\t\t\t"console": "externalTerminal",\n'
			output = output .. '\t\t\t\t"type": "cppvsdbg",\n'
			output = output .. '\t\t\t\t"request": "launch",\n'
			output = output .. string.format('\t\t\t\t"program": "${workspaceRoot}/%s",\n', programPath)
			output = output .. '\t\t\t},\n'
			output = output .. '\t\t\t"stopAtEntry": false,\n'
			output = output .. '\t\t\t"cwd": "'..(cfg.vscode_launch_cwd or string.format('${workspaceRoot}/%s', target))..'",\n'
			if prj.vscode_launch_visualizerFile ~= nil then
				output = output .. '\t\t\t"visualizerFile": "'..prj.vscode_launch_visualizerFile..'",\n'
				output = output .. '\t\t\t"showDisplayString": true,\n'
			end
			if prj.vscode_launch_args ~= nil then
				output = output .. '\t\t\t"args":[\n'
				for i,v in ipairs(prj.vscode_launch_args) do
					output = output .. '\t\t\t\t"'..v..'",\n'
				end
				output = output .. '\t\t\t],\n'
			end
			if cfg.vscode_launch_environment ~= nil then
				output = output .. '\t\t\t"environment":[\n'
				for k,v in pairs(cfg.vscode_launch_environment) do
					output = output .. '\t\t\t\t{\n'
					output = output .. '\t\t\t\t\t"name": "'..k..'",\n'
					output = output .. '\t\t\t\t\t"value": "'..v..'"\n'
					output = output .. '\t\t\t\t},\n'
				end
				output = output .. '\t\t\t],\n'
			end
			output = output .. string.format('\t\t\t"preLaunchTask": "Build %s (%s)",\n', prj.name, cfg.name)
			output = output .. '\t\t},\n'
		end
	end

	launchFile:write(output)
end

function m.vscode_c_cpp_properties(prj, propsFile)
	local cdialect = "${default}"
	if prj.cdialect then
		cdialect = string.lower(prj.cdialect)
	end

	local cppdialect = "${default}"
	if prj.cppdialect then
		cppdialect = string.lower(prj.cppdialect)
	end

	local output = ""

	for cfg in project.eachconfig(prj) do
		output = output .. '\t\t{\n'
		output = output .. string.format('\t\t\t"name": "%s (%s)",\n', prj.name, cfg.name)
		output = output .. '\t\t\t"includePath":\n'
		output = output .. '\t\t\t[\n'
		output = output .. '\t\t\t\t"${workspaceFolder}/**"'
		if #cfg.includedirs > 0 or #cfg.externalincludedirs > 0 then
			output = output .. ','
		end
		output = output .. '\n'
		for _, includedir in ipairs(cfg.includedirs) do
			output = output .. string.format('\t\t\t\t"%s",\n', includedir)
		end
		for _, includedir in ipairs(cfg.externalincludedirs) do
			output = output .. string.format('\t\t\t\t"%s",\n', includedir)
		end
		if #cfg.includedirs > 0 or #cfg.externalincludedirs > 0 then
			output = output:sub(1, -3) .. '\n'
		end
		output = output .. '\t\t\t],\n'
		output = output .. '\t\t\t"defines":\n'
		output = output .. '\t\t\t[\n'
		for i = 1, #cfg.defines do
			output = output .. string.format('\t\t\t\t"%s",\n', cfg.defines[i]:gsub('"','\\"'))
		end
		if #cfg.defines > 0 then
			output = output:sub(1, -3) .. '\n'
		end
		output = output .. '\t\t\t],\n'
		output = output .. string.format('\t\t\t"cStandard": "%s",\n', cdialect)
		output = output .. string.format('\t\t\t"cppStandard": "%s",\n', cppdialect)
		output = output .. '\t\t\t"intelliSenseMode": "${default}"\n'

		output = output .. '\t\t},\n'
	end

	propsFile:write(output)
end
