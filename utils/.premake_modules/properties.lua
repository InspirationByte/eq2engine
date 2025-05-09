local p = premake
local m = p.validation

--
-- 'property' container
--
	p.property = p.api.container("property", p.global, { "config", "project" })
	-- HACK - we need to make table otherwise it won't work
	p.api.scope.global.properties = {}

	function p.property.new(name)
		local self = p.container.new(p.property, name)
		return self
	end
	
--
-- Extend global with getProperty
--

	function p.global.getProperty(key)		
		local root = p.api.rootContainer()
		return root.properties[key]
	end
	
--
-- Extend global with eachProperty iterator
--
	
	function p.global.eachProperty()
		local root = p.api.rootContainer()
		return p.container.eachChild(root, p.property)
	end
	
--
-- applyProperty
--

	local function applyProperty( targetProject, targetBlock, property )		
		verbosef("\nProject %s is using property %s\n",
				 targetProject.name, property.name)

		-- Clone each block in the property and insert it into the target project
		for _,propertyBlock in ipairs(property.blocks) do
			-- detach fat references before deepcopy
			propertyOrigin = propertyBlock._origin
			propertyBlock._origin = nil
			
			local newBlock = table.deepcopy(propertyBlock)

			-- attach fat references after deepcopy
			newBlock._origin   = propertyOrigin
			propertyBlock._origin = propertyOrigin

			newBlock._criteria.patterns = table.join( newBlock._criteria.patterns, targetBlock._criteria.patterns )
			newBlock._criteria.data = p.criteria._compile(newBlock._criteria.patterns)

			-- todo: would be nice not to do this.
			--  	 needs to be in sync with internal block logic.
			if newBlock.filename then
				newBlock.filename = nil
				newBlock._basedir = newBlock.basedir
				newBlock.basedir = nil
			end
		
			-- Insert the new block into the target project.
			-- TODO: We need to insert as if at the call site,
			--  	 and it need to deal with with 'removes'
			--  	 merging between the two blocks.
			table.insert( targetProject.blocks, newBlock )

			-- Recursion in property is to fuzzy
			if newBlock.properties then
				error("Property '" .. property.name .. "': Properties in property is forbidden, move 'property' to project.")
			end
		end
	end 

--
-- Resolve a single 'property' into a target block of a target project
--

	local function resolveProperty(projectOrWorkspace, targetBlock, propName)
		local property = p.global.getProperty(propName)

		if property == nil then
			error("Property "
				  .. "'" .. propName.. "'"
				  .. " is not defined as a property")
		end

		if property ~= nil then
			applyProperty( projectOrWorkspace, targetBlock, property )
		end
	end

--
-- Resolve all properties from a target block in a target project
--

	local function resolveAllPropsInBlock( projectOrWorkspace, targetBlock )
		if targetBlock.properties then
			for _, propKey in ipairs(targetBlock.properties) do
				resolveProperty( projectOrWorkspace, targetBlock, propKey )
			end
		end
	end

--
-- Before baking a workspaces and projects, resolve all the 'properties'
--

	premake.override(p.workspace, "bake", function(base, self)	
		-- Keep the list stable while we iterate and modify it
		local blocks = {}
		for k, v in pairs(self.blocks) do
			blocks[k] = v
		end
		
		for _, block in pairs(blocks) do
			resolveAllPropsInBlock(self, block)
		end
	
		return base(self)
	end)

	premake.override(p.project, "bake", function(base, self)	
		-- Keep the list stable while we iterate and modify it
		local blocks = {}
		for k, v in pairs(self.blocks) do
			blocks[k] = v
		end
		
		for _, block in pairs(blocks) do
			resolveAllPropsInBlock(self, block)
		end
	
		return base(self)
	end)
	

--
-- 'properties' api
--

p.api.register {
	name = "properties",
	scope = { "config" },
	kind = "list:string",
}