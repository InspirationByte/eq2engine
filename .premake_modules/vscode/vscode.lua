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
	scope = "project",
	display = "[vscode] makefile location",
	description = "Location of the makefile",
	kind = "string",
    tokens = true,
}

p.api.register {
	name = "vscode_solution",
	scope = "project",
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
	scope = "project",
	display = "[VSCode] launch working directory",
	description = "Working directory of launched app",
	kind = "string",
    tokens = true,
}

p.api.register {
	name = "vscode_launch_environment",
	scope = "project",
	display = "[VSCode] launch environment strings",
	description = "Environment strings of launched app",
	kind = "table",
    tokens = true,
}

p.api.register {
	name = "vscode_launch_args",
	scope = "project",
	display = "[VSCode] launch arguments",
	description = "Launched app arguments",
	kind = "list:string",
    tokens = true,
}

function vscode.generateWorkspace(wks)
    -- Only create workspace file if it doesnt already exist
    if not os.isfile(wks.location .. "/" .. wks.name .. ".code-workspace") then
        local codeWorkspaceFile = io.open(wks.location .. "/" .. wks.name .. ".code-workspace", "w")
        codeWorkspaceFile:write('{\n\t"folders":\n\t[\n\t\t{\n\t\t\t"path": ".",\n\t\t},\n\t],\n}\n')
        codeWorkspaceFile:close()
    end

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
        end
    end

    propsFile:write('\t]\n}\n')
    propsFile:close()

    launchFile:write('\t]\n}\n')
    launchFile:close()

    tasksFile:write('\t]\n}\n')
    tasksFile:close()
end

include("vscode_project.lua")

include("_preload.lua")

return vscode
