--
-- Name:        vscode.lua
-- Purpose:     Define the vscode action(s).
-- Author:      Ryan Pusztai
-- Modified by: Andrea Zanellato
--              Andrew Gough
--              Manu Evans
--              Jason Perkins
--              Yehonatan Ballas
--              Jan "GamesTrap" Schürkamp
-- Created:     2013/05/06
-- Updated:     2022/12/29
-- Copyright:   (c) 2008-2020 Jason Perkins and the Premake project
--              (c) 2022-2023 Jan "GamesTrap" Schürkamp
--

local p = premake

p.modules.vscode = {}
p.modules.vscode._VERSION = p._VERSION

local vscode = p.modules.vscode
local project = p.project

p.api.register {
	name = "vscode_makefile",
	scope = "workspace",
	display = "[vscode] makefile location",
	description = "Location of the makefile",
	kind = "string",
    tokens = true,
}

p.api.register {
	name = "vscode_solution",
	scope = "workspace",
	display = "[VSCode] sln location",
	description = "Location of the solution file",
	kind = "string",
    tokens = true,
}

p.api.register {
	name = "vscode_launch_visualizerFile",
	scope = "project",
	display = "[VSCode] launchdebugger NatVis file",
	description = "Debugger NatVis file",
	kind = "string",
    tokens = true,
}

p.api.register {
	name = "vscode_launch_cwd",
	scope = "config",
	display = "[VSCode] launch working directory",
	description = "Working directory of launched app",
	kind = "string",
    tokens = true,
}

p.api.register {
	name = "vscode_launch_environment",
	scope = "config",
	display = "[VSCode] launch environment strings",
	description = "Environment strings of launched app",
	kind = "table",
    tokens = true,
}

p.api.register {
	name = "vscode_launch_args",
	scope = "config",
	display = "[VSCode] launch arguments",
	description = "Launched app arguments",
	kind = "list:string",
    tokens = true,
}

local function splitStr(instr, separator)
    if separator == nil then
        separator = "%s"
    end
    local t = {}
    for str in string.gmatch(instr, "([^".. separator .. "]+)") do
        table.insert(t, str)
    end
    return t
end

local function longestCommonPath(prj)
    local files = {}
    for _, f in ipairs(prj.files) do
        table.insert(files, f)
    end
    if #files == 0 then return './' end

    local commonParts = splitStr(files[1], "/")
    for i = 2, #files do
        local parts = splitStr(files[i], "/")
        for j = #commonParts, 1, -1 do
            if parts[j] ~= commonParts[j] then
                commonParts[j] = nil
            end
        end
    end
    local commonPartsClean = {}
    for _, v in ipairs(commonParts) do
        if v ~= nil then
            table.insert(commonPartsClean, v)
        end
    end

    local result = table.concat(commonPartsClean, "/")
    if files[1]:sub(1, 1) == "/" then
        result = "/" .. result
    end

    return result
end

function vscode.generateWorkspace(wks)
    -- Only create workspace file if it doesnt already exist

    local codeWorkspaceFile = io.open(wks.location .. "/" .. wks.name .. ".code-workspace", "w")
    codeWorkspaceFile:write('{\n\t"folders":\n\t[')
    codeWorkspaceFile:write('\n\t\t{\n\t\t\t"path": ".",\n\t\t},')

    local startString = '{\n\t"version": "%s",\n\t"%s":\n\t[\n'

    local makefileLocation = ''
    if wks.vscode_makefile ~= nil then
        makefileLocation = '/'..wks.vscode_makefile
    end

    -- Create tasks.json file
    local tasksFile = io.open(wks.location .. "/.vscode/tasks.json", "w")
    tasksFile:write('{\n\t"version": "2.0.0",\n\t"options":\n\t{\n')
    tasksFile:write(string.format('\t\t"cwd": "${workspaceRoot}%s",\n\t},\n', makefileLocation))
    tasksFile:write('\t"tasks":\n\t[\n')

    -- Create launch.json file
    local launchFile = io.open(wks.location .. "/.vscode/launch.json", "w")
    launchFile:write('{\n\t"version": "2.0.0",\n\t"configurations":\n\t[\n')

    -- Create launch.json file
    local propsFile = io.open(wks.location .. "/.vscode/c_cpp_properties.json", "w")
    propsFile:write('{\n\t"version": 4,\n\t"configurations":\n\t[\n')

    -- For each project
    for prj in p.workspace.eachproject(wks) do
        if project.isc(prj) or project.iscpp(prj) then
            vscode.project.vscode_tasks(prj, tasksFile)
            vscode.project.vscode_launch(prj, launchFile)
            vscode.project.vscode_c_cpp_properties(prj, propsFile)

            --local sourcePath = path.getrelative(prj.workspace.location, longestCommonPath(prj))
            --codeWorkspaceFile:write(string.format('\n\t\t{\n\t\t\t"name": "%s",\n\t\t\t"path": "%s",\n\t\t},', prj.name, sourcePath))
        end
    end

    propsFile:write('\t]\n}\n')
    propsFile:close()

    launchFile:write('\t]\n}\n')
    launchFile:close()

    tasksFile:write('\t]\n}\n')
    tasksFile:close()

    codeWorkspaceFile:write('\n\t],\n}\n')
    codeWorkspaceFile:close()
end

include("vscode_project.lua")

include("_preload.lua")

return vscode
